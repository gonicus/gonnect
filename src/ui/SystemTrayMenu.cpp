#include "SystemTrayMenu.h"
#include "NumberStat.h"
#include "ViewHelper.h"
#include "NumberStats.h"
#include "Contact.h"
#include "SIPManager.h"
#include "SIPCallManager.h"
#include "SIPAccountManager.h"
#include "PhoneNumberUtil.h"
#include "TogglerManager.h"
#include "Toggler.h"
#include "Application.h"

using namespace std::chrono_literals;

SystemTrayMenu::SystemTrayMenu(QObject *parent) : QObject{ parent }
{
    initMenu();
    updateCalls();
    updateFavorites();
    updateMostCalled();
    updateTogglers();

    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setContextMenu(m_trayIconMenu);
    m_trayIcon->show();
    resetTrayIcon();

    updateMenu();

    m_ringTimer.setInterval(750ms);
    connect(&m_ringTimer, &QTimer::timeout, this, &SystemTrayMenu::ringTimerCallback);

    auto numStats = &NumberStats::instance();
    connect(numStats, &NumberStats::favoriteAdded, this, &SystemTrayMenu::updateFavorites);
    connect(numStats, &NumberStats::favoriteRemoved, this, &SystemTrayMenu::updateFavorites);
    connect(numStats, &NumberStats::numberStatAdded, this, &SystemTrayMenu::updateFavorites);
    connect(numStats, &NumberStats::countChanged, this, &SystemTrayMenu::updateFavorites);

    connect(numStats, &NumberStats::numberStatAdded, this, &SystemTrayMenu::updateMostCalled);
    connect(numStats, &NumberStats::countChanged, this, &SystemTrayMenu::updateMostCalled);

    connect(&SIPManager::instance(), &SIPManager::buddyStateChanged, this,
            &SystemTrayMenu::updateBuddyState);
    connect(&SIPAccountManager::instance(), &SIPAccountManager::sipRegisteredChanged, this,
            &SystemTrayMenu::updateMenu);
    connect(&SIPCallManager::instance(), &SIPCallManager::activeCallsChanged, this,
            &SystemTrayMenu::updateCalls);
    connect(&SIPCallManager::instance(), &SIPCallManager::establishedCallsCountChanged, this,
            &SystemTrayMenu::updateCalls);
    connect(&SIPCallManager::instance(), &SIPCallManager::earlyCallStateChanged, this,
            &SystemTrayMenu::updateCalls);
    connect(&SIPCallManager::instance(), &SIPCallManager::isConferenceModeChanged, this,
            &SystemTrayMenu::updateCalls);
    connect(&SIPCallManager::instance(), &SIPCallManager::isBlockedChanged, this,
            &SystemTrayMenu::updateCalls);
    connect(&TogglerManager::instance(), &TogglerManager::togglerChanged, this,
            &SystemTrayMenu::updateTogglers);
    connect(&TogglerManager::instance(), &TogglerManager::togglerActiveChanged, this,
            &SystemTrayMenu::updateTogglers);
    connect(&TogglerManager::instance(), &TogglerManager::togglerBusyChanged, this,
            &SystemTrayMenu::updateTogglers);
}

SystemTrayMenu::~SystemTrayMenu()
{
    m_trayIconMenu->deleteLater();
}

void SystemTrayMenu::updateMenu()
{

    const auto sipReg = SIPAccountManager::instance().sipRegistered();
    m_settingsWindowAction->setVisible(sipReg);
    m_mainWindowAction->setText(sipReg ? tr("Dial...") : tr("Not registered..."));
    m_mainWindowAction->setIcon(sipReg ? QIcon::fromTheme("call-start-symbolic")
                                       : QIcon::fromTheme("view-refresh-symbolic"));

    updateCalls();
    updateFavorites();
    updateMostCalled();
}

void SystemTrayMenu::initMenu()
{
    m_trayIconMenu = new QMenu;

    QAction *action = nullptr;
    m_mainWindowAction = action =
            m_trayIconMenu->addAction(QIcon::fromTheme("call-start-symbolic"), tr("Dial..."));
    connect(action, &QAction::triggered, &ViewHelper::instance(), &ViewHelper::activateSearch);

    m_trayIconMenu->addSeparator();
    m_activeCallsSeparator = m_trayIconMenu->addSeparator();
    m_favoritesSeparator = m_trayIconMenu->addSeparator();
    m_mostCalledSeparator = m_trayIconMenu->addSeparator();
    m_togglerSeparator = m_trayIconMenu->addSeparator();

    m_settingsWindowAction = action = m_trayIconMenu->addAction(
            QIcon::fromTheme("applications-system-symbolic"), tr("Settings"));
    connect(action, &QAction::triggered, &ViewHelper::instance(), &ViewHelper::showSettings);

    action = m_trayIconMenu->addAction(QIcon::fromTheme("help-about-symbolic"), tr("About"));
    connect(action, &QAction::triggered, &ViewHelper::instance(), &ViewHelper::showAbout);

    action = m_trayIconMenu->addAction(QIcon::fromTheme("application-exit-symbolic"), tr("Quit"));
    connect(action, &QAction::triggered, &ViewHelper::instance(), &ViewHelper::quitApplication);
}

QString SystemTrayMenu::contactText(const NumberStat &numberStat) const
{
    QString result;
    QString numberStr = numberStat.phoneNumber;
    QString stateChar = "⚫";
    const auto contact = numberStat.contact;

    if (contact && numberStat.contactType == NumberStats::ContactType::PhoneNumber) {
        const auto phoneNumberObject = contact->phoneNumberObject(numberStat.phoneNumber);
        if (phoneNumberObject.isSipSubscriptable) {
            switch (SIPManager::instance().buddyStatus(numberStat.phoneNumber)) {

            case SIPBuddyState::UNKNOWN:
            case SIPBuddyState::UNAVAILABLE:
                stateChar = "⚫";
                break;
            case SIPBuddyState::BUSY:
                stateChar = "🔴";
                break;
            default:
                stateChar = "🟢";
                break;
            }
        }
    }

    if (contact && !contact->name().isEmpty()) {
        result = QString("%1  %2").arg(stateChar, contact->name()).trimmed();
    } else {
        result = QString("%1  %2").arg(stateChar, numberStr).trimmed();
    }
    return result;
}

QString SystemTrayMenu::contactIcon(const NumberStat &numberStat) const
{
    if (numberStat.contactType == NumberStats::ContactType::JitsiMeetUrl) {
        return "videochat-symbolic";
    } else if (numberStat.contact) {
        const auto phoneNumberObject =
                numberStat.contact->phoneNumberObject(numberStat.phoneNumber);

        switch (phoneNumberObject.type) {

        case Contact::NumberType::Unknown:
            break;

        case Contact::NumberType::Commercial:
            return "avatar-default-symbolic";

        case Contact::NumberType::Home:
            return "user-home-symbolic";

        case Contact::NumberType::Mobile:
            return "phone-old-symbolic";
        }
    }

    return "";
}

SystemTrayMenu::CallEntry *SystemTrayMenu::findCallEntry(const QString &remoteUri)
{
    for (auto &callEntry : m_callEntries) {
        if (callEntry.remoteUri == remoteUri) {
            return &callEntry;
        }
    }
    return nullptr;
}

void SystemTrayMenu::updateCalls()
{
    // Update call entries
    QSet<QString> activeUris;
    const auto activeCalls = SIPCallManager::instance().calls();

    // Current calls
    m_hasEstablishedCalls = false;
    for (const auto call : activeCalls) {
        const auto remoteUri = QString::fromStdString(call->getInfo().remoteUri);
        activeUris.insert(remoteUri);

        SystemTrayMenu::CallEntry *entry = findCallEntry(remoteUri);
        if (!entry) {
            m_callEntries.append({ remoteUri });
            entry = &m_callEntries.last();
        }

        entry->isEstablished = call->isEstablished();
        entry->isEarlyCallState = call->earlyCallState();

        if (call->isEstablished()) {
            m_hasEstablishedCalls = true;
        }
    }

    // Find finished calls
    for (auto &callEntry : m_callEntries) {
        if (!activeUris.contains(callEntry.remoteUri)) {
            callEntry.isFinished = true;

            if (m_callEntries.size()) {
                QTimer::singleShot(GONNECT_CALL_VISIBLE_AFTER_END, this, [this, &callEntry]() {
                    bool changed = false;

                    QMutableListIterator it(m_callEntries);
                    while (it.hasNext()) {
                        it.next();

                        if (it.value().remoteUri == callEntry.remoteUri) {
                            it.remove();
                            changed = true;
                        }
                    }

                    if (changed) {
                        updateCalls();
                    }
                });
            }
        }
    }

    // Update actions
    for (QAction *action : std::as_const(m_activeCallsActions)) {
        m_trayIconMenu->removeAction(action);
    }
    m_activeCallsActions.clear();

    if (SIPAccountManager::instance().sipRegistered()) {
        if (SIPCallManager::instance().isConferenceMode()) {
            auto action = new QAction(QIcon::fromTheme("call-stop-symbolic"), tr("End conference"),
                                      m_trayIconMenu);
            m_trayIconMenu->insertAction(m_activeCallsSeparator, action);
            m_activeCallsActions.append(action);

            connect(action, &QAction::triggered, this,
                    []() { SIPCallManager::instance().endConference(); });

        } else {
            for (const auto &callEntry : std::as_const(m_callEntries)) {

                const auto call = SIPCallManager::instance().findCall(callEntry.remoteUri);

                if (call && call->isBlocked()) {
                    continue;
                }

                const auto callInfo =
                        PhoneNumberUtil::instance().contactInfoBySipUrl(callEntry.remoteUri);
                QString contactLabel;

                if (callInfo.contact) {
                    contactLabel = callInfo.contact->name();
                }
                if (contactLabel.isEmpty()) {
                    contactLabel = callInfo.phoneNumber;
                }

                if (callEntry.isFinished || !call) {
                    auto action = new QAction(QIcon::fromTheme("call-stop-symbolic"),
                                              tr("Call with %1 has ended").arg(contactLabel),
                                              m_trayIconMenu);
                    action->setEnabled(false);
                    m_trayIconMenu->insertAction(m_activeCallsSeparator, action);
                    m_activeCallsActions.append(action);

                } else if (!callEntry.isEstablished && !callEntry.isEarlyCallState) {
                    auto action = new QAction(QIcon::fromTheme("call-start-symbolic"),
                                              tr("Accept call with %1").arg(contactLabel),
                                              m_trayIconMenu);
                    m_trayIconMenu->insertAction(m_activeCallsSeparator, action);
                    m_activeCallsActions.append(action);

                    connect(action, &QAction::triggered, this,
                            [call]() { SIPCallManager::instance().acceptCall(call); });

                } else {
                    auto action = new QAction(QIcon::fromTheme("call-stop-symbolic"),
                                              tr("Hang up call with %1").arg(contactLabel),
                                              m_trayIconMenu);
                    m_trayIconMenu->insertAction(m_activeCallsSeparator, action);
                    m_activeCallsActions.append(action);

                    connect(action, &QAction::triggered, this, [call]() {
                        if (call->isEstablished() || call->earlyCallState()) {
                            SIPCallManager::instance().endCall(call);
                        } else {
                            SIPCallManager::instance().rejectCall(call);
                        }
                    });
                }
            }
        }
    }

    m_activeCallsSeparator->setVisible(m_activeCallsActions.size());

    if (m_trayIcon) {
        resetTrayIcon();
    }
}

void SystemTrayMenu::updateFavorites()
{
    for (QAction *action : std::as_const(m_favoriteActions)) {
        m_trayIconMenu->removeAction(action);
    }
    m_favoriteActions.clear();

    if (SIPAccountManager::instance().sipRegistered()) {
        const auto favs = NumberStats::instance().favorites();
        m_favoriteActions.reserve(favs.size());

        for (const auto item : favs) {
            const auto number = item->phoneNumber;
            auto action = new QAction(QIcon::fromTheme(contactIcon(*item)), contactText(*item),
                                      m_trayIconMenu);
            m_trayIconMenu->insertAction(m_favoritesSeparator, action);
            m_favoriteActions.insert(number, action);
            const bool isJitsiMeet = item->contactType == NumberStats::ContactType::JitsiMeetUrl;
            connect(action, &QAction::triggered, this, [number, isJitsiMeet]() {
                if (isJitsiMeet) {
                    ViewHelper::instance().requestMeeting(number);
                } else {
                    SIPCallManager::instance().call(number);
                }
            });
        }
    }

    m_favoritesSeparator->setVisible(m_favoriteActions.size());
}

void SystemTrayMenu::updateMostCalled()
{
    for (QAction *action : std::as_const(m_mostCalledActions)) {
        m_trayIconMenu->removeAction(action);
    }
    m_mostCalledActions.clear();

    if (SIPAccountManager::instance().sipRegistered()) {

        const auto mostCalled = NumberStats::instance().mostCalled(5, false);
        m_mostCalledActions.reserve(mostCalled.size());

        for (const auto &number : mostCalled) {
            const auto &numStat = *(NumberStats::instance().numberStat(number));
            auto action = new QAction(QIcon::fromTheme(contactIcon(numStat)), contactText(numStat),
                                      m_trayIconMenu);
            m_trayIconMenu->insertAction(m_mostCalledSeparator, action);
            m_mostCalledActions.insert(number, action);
            connect(action, &QAction::triggered, this,
                    [number]() { SIPCallManager::instance().call(number); });
        }
    }

    m_mostCalledSeparator->setVisible(m_mostCalledActions.size());
}

void SystemTrayMenu::updateTogglers()
{
    for (QAction *action : std::as_const(m_togglerActions)) {
        m_trayIconMenu->removeAction(action);
    }
    m_togglerActions.clear();

    const auto togglers = TogglerManager::instance().toggler();
    for (const auto toggler : togglers) {
        if (toggler->display() & Toggler::DISPLAY::TRAY) {
            auto action = new QAction(toggler->name(), m_trayIconMenu);

            if (toggler->isBusy()) {
                action->setEnabled(false);
                action->setIcon(QIcon::fromTheme("view-refresh-symbolic"));
            } else {
                action->setCheckable(true);
                action->setChecked(toggler->isActive());
            }

            m_trayIconMenu->insertAction(m_togglerSeparator, action);
            m_togglerActions.append(action);
            connect(action, &QAction::triggered, this,
                    [toggler]() { TogglerManager::instance().toggleToggler(toggler->id()); });
        }
    }

    m_togglerSeparator->setVisible(m_togglerActions.size());
}

void SystemTrayMenu::updateBuddyState(const QString uri, SIPBuddyState::STATUS)
{
    const auto number = PhoneNumberUtil::numberFromSipUrl(uri);

    if (auto action = m_favoriteActions.value(number, nullptr)) {
        action->setText(contactText(*(NumberStats::instance().numberStat(number))));
    }
    if (auto action = m_mostCalledActions.value(number, nullptr)) {
        action->setText(contactText(*(NumberStats::instance().numberStat(number))));
    }
}

void SystemTrayMenu::setRinging(bool flag)
{
    if (flag) {
        m_ringTimer.start();
    } else {
        m_ringTimer.stop();
        resetTrayIcon();
    }
}

void SystemTrayMenu::ringTimerCallback()
{
    QString noteDot = m_missedCallsCount ? "_note" : "";

    m_ringingState = !m_ringingState;

    if (m_ringingState) {
        m_trayIcon->setIcon(QIcon(":/icons/gonnect_ring" + noteDot + ".svg"));
    } else {
        resetTrayIcon();
    }
}

void SystemTrayMenu::resetTrayIcon()
{
    QString noteDot = m_missedCallsCount ? "_note" : "";

    if (m_hasEstablishedCalls) {
        m_trayIcon->setIcon(QIcon(":/icons/gonnect_line" + noteDot + ".svg"));
    } else {
        if (m_settings.value("generic/trayIconDark", false).toBool()) {
            m_trayIcon->setIcon(QIcon(":/icons/gonnect_dark" + noteDot + ".svg"));
        } else {
            m_trayIcon->setIcon(QIcon(":/icons/gonnect_light" + noteDot + ".svg"));
        }
    }
}

void SystemTrayMenu::setBadgeNumber(unsigned number)
{
    m_missedCallsCount = number;
    resetTrayIcon();
}
