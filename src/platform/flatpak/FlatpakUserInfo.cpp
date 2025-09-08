#include "FlatpakUserInfo.h"
#include "UserInfo.h"
#include "AccountPortal.h"
#include "AppSettings.h"
#include "ReadOnlyConfdSettings.h"

UserInfo &UserInfo::instance()
{
    static UserInfo *_instance = nullptr;
    if (!_instance) {
        _instance = new FlatpakUserInfo;
    }
    return *_instance;
}

FlatpakUserInfo::FlatpakUserInfo() : UserInfo{}
{
    ReadOnlyConfdSettings settings;

    m_accountPortal = new AccountPortal(this);
    m_displayName = settings.value("generic/displayName", "").toString();
}

QString FlatpakUserInfo::getDisplayName()
{
    if (m_displayName.isEmpty()) {
        acquireDisplayName([this](const QString &displayName) {
            m_displayName = displayName;
            emit displayNameChanged();
        });
    }

    return m_displayName;
}

void FlatpakUserInfo::acquireDisplayName(std::function<void(const QString &displayName)> callback)
{
    if (m_displayName.isEmpty()) {
        m_accountPortal->GetUserInformation(
                tr("The UC client wants to use your name to configure your display name."),
                [callback](uint code, const QVariantMap &response) {
                    QString username;

                    if (code == 0) {
                        username = response.value("name", "").toString();
                        if (!username.isEmpty()) {
                            AppSettings settings;
                            settings.setValue("generic/displayName", username);
                        }
                    }

                    callback(username);
                });

        return;
    }

    callback(m_displayName);
}
