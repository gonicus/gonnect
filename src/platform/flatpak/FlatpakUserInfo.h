#pragma once
#include <QObject>
#include "../UserInfo.h"

class AccountPortal;

class FlatpakUserInfo : public UserInfo
{
    Q_OBJECT

public:
    explicit FlatpakUserInfo();
    QString getDisplayName() override;

private:
    void acquireDisplayName(std::function<void(const QString &displayName)> callback);

    AccountPortal *m_accountPortal = nullptr;
    QString m_displayName;
};
