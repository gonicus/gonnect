#pragma once
#include <QObject>
#include "../UserInfo.h"

class WindowsUserInfo : public UserInfo
{
    Q_OBJECT

public:
    explicit WindowsUserInfo();
    QString getDisplayName() override;
};
