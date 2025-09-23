#pragma once
#include <QObject>
#include "../UserInfo.h"

class DummyUserInfo : public UserInfo
{
    Q_OBJECT

public:
    explicit DummyUserInfo() : UserInfo() { }
    QString getDisplayName() override { return QString(); }
};
