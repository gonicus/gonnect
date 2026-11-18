#include "ConferenceVideoModel.test.h"
#include "ConferenceVideoModel.h"
#include "ConferenceVideoOutput.h"

#include <QAbstractItemModelTester>
#include <QCoreApplication>
#include <QImage>
#include <QPointer>
#include <QSignalSpy>
#include <QTest>
#include <QVideoFrame>
#include <QVideoSink>

namespace {

using Type = ConferenceVideoSource::Type;
using Roles = ConferenceVideoModel::Roles;

const QString UserId = QStringLiteral("fd2257db");
const QString CameraId = QStringLiteral("fd2257db-v0");
const QString ScreenId = QStringLiteral("fd2257db-v1");

QVariant roleData(const ConferenceVideoModel &model, int row, Roles role)
{
    return model.data(model.index(row), static_cast<int>(role));
}

void processDeferredDeletes()
{
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
}

} // namespace

void ConferenceVideoModelTest::addsAndRemovesSources()
{
    ConferenceVideoModel model;
    const QAbstractItemModelTester tester(&model,
                                          QAbstractItemModelTester::FailureReportingMode::QtTest);

    auto *camera = model.addSource(CameraId, UserId, Type::Camera);
    auto *screen = model.addSource(ScreenId, UserId, Type::ScreenShare);

    QCOMPARE(model.rowCount(), 2);
    QCOMPARE(roleData(model, 0, Roles::Source).value<ConferenceVideoSource *>(), camera);
    QCOMPARE(roleData(model, 1, Roles::UserId).toString(), UserId);
    QCOMPARE(roleData(model, 1, Roles::Type).value<Type>(), Type::ScreenShare);
    QCOMPARE(model.source(ScreenId), screen);

    model.removeSource(CameraId);

    QCOMPARE(model.rowCount(), 1);
    QVERIFY(!model.source(CameraId));
    QCOMPARE(roleData(model, 0, Roles::Source).value<ConferenceVideoSource *>(), screen);
}

void ConferenceVideoModelTest::addSourceIsIdempotent()
{
    ConferenceVideoModel model;

    auto *first = model.addSource(CameraId, UserId, Type::Camera);
    auto *second = model.addSource(CameraId, UserId, Type::Camera);

    QCOMPARE(first, second);
    QCOMPARE(model.rowCount(), 1);
}

void ConferenceVideoModelTest::reportsMuteChange()
{
    ConferenceVideoModel model;
    const QAbstractItemModelTester tester(&model,
                                          QAbstractItemModelTester::FailureReportingMode::QtTest);

    model.addSource(CameraId, UserId, Type::Camera);
    auto *screen = model.addSource(ScreenId, UserId, Type::ScreenShare);
    QSignalSpy spy(&model, &QAbstractItemModel::dataChanged);

    screen->setIsMuted(true);
    screen->setIsMuted(true);

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).value<QModelIndex>().row(), 1);
    QCOMPARE(roleData(model, 1, Roles::IsMuted).toBool(), true);
}

void ConferenceVideoModelTest::clearsAllSources()
{
    ConferenceVideoModel model;
    const QPointer<ConferenceVideoSource> camera = model.addSource(CameraId, UserId, Type::Camera);

    model.clear();
    QCOMPARE(model.rowCount(), 0);

    processDeferredDeletes();
    QVERIFY(camera.isNull());
}

void ConferenceVideoModelTest::keepsFramePumpAliveForDecoder()
{
    ConferenceVideoModel model;
    const QPointer<ConferenceVideoSource> camera = model.addSource(CameraId, UserId, Type::Camera);

    std::shared_ptr<FramePump> decoderPump = camera->framePump();
    const QPointer<FramePump> pump = decoderPump.get();

    model.removeSource(CameraId);
    processDeferredDeletes();
    QVERIFY(camera.isNull());
    QVERIFY(!pump.isNull());

    decoderPump.reset();
    processDeferredDeletes();
    QVERIFY(pump.isNull());
}

void ConferenceVideoModelTest::outputFeedsSinkAndReportsHeight()
{
    ConferenceVideoModel model;
    auto *camera = model.addSource(CameraId, UserId, Type::Camera);
    QSignalSpy heightSpy(&model, &ConferenceVideoModel::requestedHeightsChanged);
    QVideoSink sink;

    {
        ConferenceVideoOutput output;
        output.setDisplayedHeight(180);
        output.setVideoSink(&sink);
        output.setSource(camera);
        QCOMPARE(camera->requestedHeight(), 180);

        QImage image(4, 2, QImage::Format_RGB32);
        image.fill(Qt::red);
        camera->framePump()->push(QVideoFrame(image));
        QTRY_VERIFY(sink.videoFrame().isValid());
    }

    // A destroyed output no longer requests video
    QCOMPARE(camera->requestedHeight(), 0);
    QCOMPARE(heightSpy.count(), 2);
}

QTEST_GUILESS_MAIN(ConferenceVideoModelTest)
