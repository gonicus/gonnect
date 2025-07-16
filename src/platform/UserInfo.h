#pragma once
#include <QObject>

class AccountPortal;

class UserInfo : public QObject
{
    Q_OBJECT

public:
    static UserInfo &instance()
    {
        static UserInfo *_instance = nullptr;
        if (!_instance) {
            _instance = new UserInfo;
        }
        return *_instance;
    }

    QString getDisplayName();

private:
    explicit UserInfo(QObject *parent = nullptr);
    void acquireDisplayName(std::function<void(const QString &displayName)> callback);

    AccountPortal *m_accountPortal = nullptr;
    QString m_displayName;

signals:
    void displayNameChanged();
};
