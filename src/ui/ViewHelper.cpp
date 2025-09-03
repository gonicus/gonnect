#include "ViewHelper.h"
#include "AddressBook.h"
#include "AddressBookManager.h"
#include "Contact.h"
#include "Application.h"
#include "NumberStats.h"
#include "ReadOnlyConfdSettings.h"
#include "Ringer.h"
#include "AuthManager.h"
#include "GlobalCallState.h"
#include "USBDevices.h"
#include "HeadsetDeviceProxy.h"
#include "SystemTrayMenu.h"
#include "SIPCallManager.h"
#include "GlobalCallState.h"
#include "DateEventManager.h"
#include "DateEvent.h"

#include <QApplication>
#include <QClipboard>
#include <QMediaFormat>
#include <QFileDialog>
#include <QSystemTrayIcon>
#include <QRegularExpression>
#include <QUuid>
#include <QMimeType>
#include <QLoggingCategory>

using namespace std::chrono_literals;

Q_LOGGING_CATEGORY(lcViewHelper, "gonnect.app.ui.ViewHelper")

ViewHelper::ViewHelper(QObject *parent) : QObject{ parent }
{
    connect(&GlobalCallState::instance(), &GlobalCallState::globalCallStateChanged, this,
            &ViewHelper::updateIsActiveVideoCall);

    m_ringerTimer.setSingleShot(true);
    m_ringerTimer.setInterval(3s);
    m_ringerTimer.callOnTimeout(this, &ViewHelper::stopTestPlayRingTone);

    connect(&AddressBook::instance(), &AddressBook::contactsReady, this,
            &ViewHelper::updateCurrentUser);
    connect(&AddressBook::instance(), &AddressBook::contactsCleared, this,
            &ViewHelper::updateCurrentUser);
    updateCurrentUser();

    // React on Teams button
    auto &hds = USBDevices::instance();
    auto dev = hds.getHeadsetDeviceProxy();
    connect(dev, &HeadsetDeviceProxy::teamsButton, this, &ViewHelper::activateSearch);
}

bool ViewHelper::isSystrayAvailable() const
{
    return QSystemTrayIcon::isSystemTrayAvailable();
}

bool ViewHelper::isJitsiAvailable() const
{
    static bool _isJitsiAvailable = false;
    static bool _isInitialized = false;

    if (!_isInitialized) {
        _isInitialized = true;

        ReadOnlyConfdSettings settings;
        _isJitsiAvailable = settings.childGroups().contains("jitsi");
    }

    return _isJitsiAvailable;
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

bool ViewHelper::isToday(const QDate date) const
{
    return date == QDate::currentDate();
}

bool ViewHelper::isTomorrow(const QDate date) const
{
    return date == QDate::currentDate().addDays(1);
}

QString ViewHelper::minutesToNiceText(uint minutes) const
{
    const uint hours = minutes / 60;
    const uint mins = minutes % 60;

    if (hours == 0) {
        return tr("%n minute(s)", "", mins);
    }
    if (hours == 1) {
        return tr("1 hour and %n minute(s)", "", mins);
    }
    return tr("%n hour(s)", "", hours);
}

QString ViewHelper::secondsToNiceText(int seconds) const
{
    const auto hours = seconds / (60 * 60);

    if (hours > 0) {
        return QString::asprintf("%01u:%02u:%02u", seconds / (60 * 60), (seconds % (60 * 60)) / 60,
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

Contact *ViewHelper::contactIdBySourceUid(const QString &sourceUid) const
{
    return AddressBook::instance().lookupBySourceUid(sourceUid);
}

QString ViewHelper::currentUserName() const
{
    QString name;
    if (m_currentUser) {
        name = m_currentUser->name();
    } else {
        AppSettings settings;
        name = settings.value("generic/displayName").toString();
    }
    return name;
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

void ViewHelper::toggleFavorite(const QString &phoneNumber,
                                const NumberStats::ContactType contactType) const
{
    NumberStats::instance().toggleFavorite(phoneNumber, contactType);
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

void ViewHelper::updateCurrentUser()
{
    Contact *contact = AddressBook::instance().lookupBySourceUid(AuthManager::instance().userUid());

    if (m_currentUser != contact) {
        m_currentUser = contact;
        emit currentUserChanged();
    }
}

void ViewHelper::updateIsActiveVideoCall()
{
    bool hasJitsiCall = false;
    const auto &callObjects = GlobalCallState::instance().globalCallStateObjects();

    for (const auto obj : callObjects) {
        const auto jitsiConn = qobject_cast<JitsiConnector *>(obj);
        if (jitsiConn && jitsiConn->callState() & ICallState::State::CallActive) {
            hasJitsiCall = true;
            break;
        }
    }

    if (m_isActiveVideoCall != hasJitsiCall) {
        m_isActiveVideoCall = hasJitsiCall;
        emit isActiveVideoCallChanged();
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
    if (GlobalCallState::instance().globalCallState() & ICallState::State::CallActive) {
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
    return USBDevices::instance().getHeadsetDeviceProxy();
}

void ViewHelper::requestPassword(const QString &id, const QString &host)
{
    emit passwordRequested(id, host);
}

void ViewHelper::respondPassword(const QString &id, const QString password)
{
    emit passwordResponded(id, password);
}

const QString ViewHelper::requestUserVerification(const QString &verificationKey)
{
    const QString uuid = QUuid::createUuid().toString();
    emit userVerificationRequested(uuid, verificationKey);
    return uuid;
}

void ViewHelper::respondUserVerification(const QString &id, bool isAccepted)
{
    emit userVerificationResponded(id, isAccepted);
}

bool ViewHelper::isPhoneNumber(const QString &number) const
{
    static const QRegularExpression numberRegEx(R"(^[+#*0-9 ]+$)");
    return numberRegEx.match(number).hasMatch();
}

bool ViewHelper::isValidJitsiRoomName(const QString &name) const
{
    // Regex from Jitsi Meet source code
    static const QRegularExpression roomNameRegEx(R"(^[^?&:"'%#]+$)");
    return roomNameRegEx.match(name).hasMatch();
}

void ViewHelper::requestMeeting(const QString &roomName, QPointer<CallHistoryItem> callHistoryItem,
                                const QString &displayName)
{
    qCInfo(lcViewHelper).nospace().noquote()
            << "Requesting meeting for " << roomName << " (" << displayName
            << ") with flags:" << m_nextMeetingStartFlags;

    emit openMeetingRequested(roomName, displayName, m_nextMeetingStartFlags, callHistoryItem);
    setProperty("nextMeetingStartFlags",
                QVariant::fromValue(JitsiConnector::MeetingStartFlag::AudioActive));
}

void ViewHelper::setCallInForegroundByIds(const QString &accountId, int callId)
{
    setProperty("callInForeground",
                QVariant::fromValue(SIPCallManager::instance().findCall(accountId, callId)));
}

bool ViewHelper::hasNonSilentCall() const
{
    const auto globalStateObject = GlobalCallState::instance().globalCallStateObjects();
    for (const auto stateObj : globalStateObject) {
        const auto callObj = qobject_cast<const SIPCall *>(stateObj);
        if (!callObj || !callObj->isSilent()) {
            return false;
        }
    }

    return true;
}

bool ViewHelper::isBusyOnBusy() const
{
    ReadOnlyConfdSettings settings;
    return settings.value("generic/busyOnBusy", false).toBool();
}

bool ViewHelper::hasOngoingDateEventByRoomName(const QString &roomName) const
{
    return DateEventManager::instance().currentDateEventByRoomName(roomName);
}

QDateTime ViewHelper::endTimeForOngoingDateEventByRoomName(const QString &roomName) const
{
    const auto dateEvent = DateEventManager::instance().currentDateEventByRoomName(roomName);
    if (dateEvent) {
        return dateEvent->end();
    }
    return QDateTime();
}

void ViewHelper::toggleFullscreen()
{
    emit fullscreenToggle();
}
