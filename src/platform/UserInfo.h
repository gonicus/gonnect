#pragma once
#include <QObject>

class UserInfo : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(UserInfo)

public:
    static UserInfo &instance();

    explicit UserInfo() : QObject() { }
    ~UserInfo() = default;

    virtual QString getDisplayName() = 0;

Q_SIGNALS:
    void displayNameChanged();
};
