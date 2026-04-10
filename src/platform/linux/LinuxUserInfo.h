#pragma once
#include <QObject>
#include "../UserInfo.h"

class LinuxUserInfo : public UserInfo
{
    Q_OBJECT

public:
    explicit LinuxUserInfo();
    QString getDisplayName() override;
};
