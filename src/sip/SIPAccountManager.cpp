#include <QLoggingCategory>
#include <QRegularExpression>

#include "SIPAccountManager.h"

Q_LOGGING_CATEGORY(lcSIPAccountManager, "gonnect.sip.accounts")

SIPAccountManager::SIPAccountManager(QObject *parent) : QObject(parent) { }

void SIPAccountManager::setSipRegistered(bool value)
{
    if (m_sipRegistered != value) {
        m_sipRegistered = value;
        emit sipRegisteredChanged();
    }
}

void SIPAccountManager::updateSipRegistered()
{
    for (const auto account : std::as_const(m_accounts)) {
        if (account->isRegistered()) {
            setSipRegistered(true);
            return;
        }
    }
    setSipRegistered(false);
}

void SIPAccountManager::initialize()
{
    static QRegularExpression isAccountGroup = QRegularExpression("^account[0-9]+$");

    // Look for accountN groups
    QStringList groups = m_settings.childGroups();
    for (auto &group : std::as_const(groups)) {

        if (isAccountGroup.match(group).hasMatch()) {
            auto sipAccount = new SIPAccount(group, this);

            connect(sipAccount, &SIPAccount::isRegisteredChanged, this,
                    &SIPAccountManager::updateSipRegistered);
            connect(sipAccount, &SIPAccount::authorizationFailed, this,
                    [this, sipAccount]() { emit authorizationFailed(sipAccount->id()); });
            updateSipRegistered();

            if (sipAccount->initialize()) {
                qCInfo(lcSIPAccountManager) << "created account" << group;
                m_accounts.push_back(sipAccount);
            } else {
                qCCritical(lcSIPAccountManager)
                        << "skipped" << group << "due to initialization errors";
            }
        }
    }

    emit accountsChanged();
}

void SIPAccountManager::setAccountCredentials(const QString &accountId, const QString &password)
{
    if (auto account = getAccount(accountId)) {
        account->setCredentials(password);
    } else {
        qCCritical(lcSIPAccountManager) << "account" << accountId << "not found";
    }
}

uint SIPAccountManager::sipRegisterRetryInterval() const
{
    for (const auto account : std::as_const(m_accounts)) {
        return account->retryInterval();
    }
    return 30;
}

SIPAccount *SIPAccountManager::getAccount(const QString &accountId)
{
    for (SIPAccount *ac : std::as_const(m_accounts)) {
        if (ac->id() == accountId) {
            return ac;
        }
    }

    return nullptr;
}
