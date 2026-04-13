#pragma once

#include <QObject>
#include <QSystemTrayIcon>
#include <QMenu>

#include "SIPBuddy.h"
#include "AppSettings.h"

struct NumberStat;

class SystemTrayMenu : public QObject
{
    Q_OBJECT

public:
    virtual ~SystemTrayMenu();

    static SystemTrayMenu &instance()
    {
        static SystemTrayMenu *_instance = nullptr;
        if (!_instance) {
            _instance = new SystemTrayMenu;
        }
        return *_instance;
    }

    Q_INVOKABLE void setBadgeNumber(unsigned number);
    void resetTrayIcon();
    void setRinging(bool flag);

private Q_SLOTS:
    void updateMenu();
    void updateCalls();
    void updateFavorites();
    void updateMostCalled();
    void updateTogglers();
    void updateBuddyState(const QString uri, SIPBuddyState::STATUS status);
    void ringTimerCallback();

private:
    struct CallEntry
    {
        QString remoteUri;
        bool isFinished = false;
        bool isEstablished = false;
        bool isEarlyCallState = false;
    };

    explicit SystemTrayMenu(QObject *parent = nullptr);
    void initMenu();

    CallEntry *findCallEntry(const QString &remoteUri);

    QString contactText(const NumberStat &numberStat) const;
    QString contactIcon(const NumberStat &numberStat) const;

    QTimer m_ringTimer;

    QSystemTrayIcon *m_trayIcon = nullptr;
    QMenu *m_trayIconMenu = nullptr;

    QAction *m_mainWindowAction = nullptr;
    QAction *m_settingsWindowAction = nullptr;
    QAction *m_activeCallsSeparator = nullptr;
    QAction *m_mostCalledSeparator = nullptr;
    QAction *m_favoritesSeparator = nullptr;
    QAction *m_togglerSeparator = nullptr;

    QList<CallEntry> m_callEntries;
    QList<QAction *> m_activeCallsActions;
    QList<QAction *> m_togglerActions;
    QHash<QString, QAction *> m_favoriteActions;
    QHash<QString, QAction *> m_mostCalledActions;

    AppSettings m_settings;

    unsigned m_notificationCount = 0;

    bool m_ringingState = false;
    bool m_hasEstablishedCalls = false;
};

class SystemTrayMenuWrapper
{
    Q_GADGET
    QML_FOREIGN(SystemTrayMenu)
    QML_NAMED_ELEMENT(SystemTrayMenu)
    QML_SINGLETON

public:
    static SystemTrayMenu *create(QQmlEngine *, QJSEngine *) { return &SystemTrayMenu::instance(); }

private:
    SystemTrayMenuWrapper() = default;
};
