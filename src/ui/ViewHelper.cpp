#include "ViewHelper.h"
#include "AddressBook.h"
#include "AddressBookManager.h"
#include "Contact.h"
#include "Application.h"
#include "NumberStats.h"
#include "Ringer.h"
#include "SIPCallManager.h"
#include "HeadsetDevices.h"
#include "HeadsetDeviceProxy.h"
#include "SecretPortal.h"
#include "SystemTrayMenu.h"

#include <QApplication>
#include <QClipboard>
#include <QMediaFormat>
#include <QFileDialog>
#include <QSystemTrayIcon>

using namespace std::chrono_literals;

ViewHelper::ViewHelper(QObject *parent) : QObject{ parent }
{
    m_ringerTimer.setSingleShot(true);
    m_ringerTimer.setInterval(3s);
    m_ringerTimer.callOnTimeout(this, &ViewHelper::stopTestPlayRingTone);
}

bool ViewHelper::isSystrayAvailable() const
{
    return QSystemTrayIcon::isSystemTrayAvailable();
}

void ViewHelper::downloadDebugInformation() const
{
    const auto fileName = Application::logFileName();

    const auto filePath =
            QDir::cleanPath(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)
                            + QDir::separator() + fileName);

    // Show file picker if we're on a desktop system
    const QString outputPath =
            QFileDialog::getSaveFileName(nullptr, tr("Save File"), filePath, "txt (*.txt)");

    QFile::copy(Application::logFilePath(), outputPath);
}

QString ViewHelper::secondsToNiceText(int seconds) const
{

    const auto hours = seconds / (60 * 60);

    if (hours > 0) {
        return QString::asprintf("%02u:%02u:%02u", seconds / (60 * 60), (seconds % (60 * 60)) / 60,
                                 (seconds % (60 * 60)) % 60);
    }

    return QString::asprintf("%02u:%02u", (seconds % (60 * 60)) / 60, (seconds % (60 * 60)) % 60);
}

int ViewHelper::secondsDelta(const QDateTime &start, const QDateTime &end) const
{
    return start.secsTo(end);
}

void ViewHelper::copyToClipboard(const QString &str) const
{
    auto clipboard = QApplication::clipboard();
    clipboard->setText(str, QClipboard::Clipboard);
    clipboard->setText(str, QClipboard::Selection);
}

void ViewHelper::reloadAddressBook() const
{
    AddressBookManager::instance().reloadAddressBook();
}

QString ViewHelper::contactIdByNumber(const QString &phoneNumber) const
{
    Contact *contact = AddressBook::instance().lookupByNumber(phoneNumber);
    return contact ? contact->id() : "";
}

QStringList ViewHelper::audioFileSelectors() const
{
    QStringList result;
    QStringList allSuffixes;
    const auto formats = QMediaFormat().supportedFileFormats(QMediaFormat::Decode);

    for (const auto format : formats) {
        QMediaFormat mediaFormat(format);

        const auto mimeType = mediaFormat.mimeType();
        if (mimeType.isValid()) {
            const QString filter = QMediaFormat::fileFormatDescription(format);
            const auto suffixes = mimeType.suffixes();

            QStringList globs;
            globs.reserve(suffixes.length());

            for (const auto &suffix : suffixes) {
                globs.append(QString("*.%1").arg(suffix));
            }
            allSuffixes.append(globs);
            result.append(QString("%1 (%2)").arg(filter, globs.join(QChar(' '))));
        }
    }

    std::sort(result.begin(), result.end());

    result.push_front(tr("Audio Files (%1)").arg(allSuffixes.join(QChar(' '))));

    return result;
}

void ViewHelper::toggleFavorite(const QString &phoneNumber) const
{
    NumberStats::instance().toggleFavorite(phoneNumber);
}

QString ViewHelper::initials(const QString &name) const
{
    const auto splitted = name.split(QChar(' '));
    if (name.length() == 0 || splitted.length() == 0) {
        return "";
    } else if (splitted.length() == 1) {
        return name.at(0);
    }

    return QString("%1%2").arg(name.at(0), splitted.at(splitted.length() - 1).at(0)).toUpper();
}

void ViewHelper::stopTestPlayRingTone()
{
    if (m_isPlayingRingTone) {
        m_ringerTimer.stop();
        m_ringer->stop();
        m_ringer->deleteLater();
        m_ringer = nullptr;

        m_isPlayingRingTone = false;
        emit isPlayingRingToneChanged();
    }
}

void ViewHelper::quitApplicationNoConfirm() const
{
    Application::instance()->quit();
}

void ViewHelper::resetTrayIcon() const
{
    SystemTrayMenu::instance().resetTrayIcon();
}

void ViewHelper::quitApplication()
{
    if (SIPCallManager::instance().hasActiveCalls()) {
        emit showQuitConfirm();
    } else {
        quitApplicationNoConfirm();
    }
}

void ViewHelper::testPlayRingTone(qreal volume)
{
    stopTestPlayRingTone();

    m_ringer = new Ringer(this);
    m_ringer->start(volume);
    m_ringerTimer.start();

    if (!m_isPlayingRingTone) {
        m_isPlayingRingTone = true;
        emit isPlayingRingToneChanged();
    }
}

HeadsetDeviceProxy *ViewHelper::headsetDeviceProxy() const
{
    return HeadsetDevices::instance().getProxy();
}

QString ViewHelper::encryptSecret(const QString &secret) const
{
    auto &sp = SecretPortal::instance();
    Q_ASSERT(sp.isValid());
    return sp.encrypt(secret);
}

void ViewHelper::requestCardDavPassword(const QString &id, const QString &host)
{
    emit cardDavPasswordRequested(id, host);
}

void ViewHelper::respondCardDavPassword(const QString &id, const QString password)
{
    emit cardDavPasswordResponded(id, password);
}
