#pragma once

#include <QObject>
#include <QQmlEngine>
#include "MainPageSelection.h"
#include "IChatRoom.h"

class ICallState;

class SelectionState : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(SelectionState)

    Q_PROPERTY(
            bool isMainWindowActive READ isMainWindowActive NOTIFY isMainWindowActiveChanged FINAL)
    Q_PROPERTY(
            MainPageSelection selectedPage MEMBER m_selectedPage NOTIFY selectedPageChanged FINAL)
    Q_PROPERTY(IChatRoom *selectedChatRoom READ selectedChatRoom WRITE setSelectedChatRoom NOTIFY
                       selectedChatRoomChanged FINAL)
    Q_PROPERTY(ICallState *callInForeground READ callInForeground WRITE setCallInForeground NOTIFY
                       callInForegroundChanged FINAL)

public:
    static SelectionState &instance()
    {
        static SelectionState s;
        return s;
    }

    Q_INVOKABLE static QString homePageId() { return "page_home"; }
    Q_INVOKABLE static QString callPageId() { return "page_call"; }
    Q_INVOKABLE static QString chatsPageId() { return "page_chats"; }
    Q_INVOKABLE static QString conferencePageId() { return "page_conference"; }
    Q_INVOKABLE static QString settingsPageId() { return "page_settings"; }

    bool isMainWindowActive() const { return m_isMainWindowActive; }
    Q_INVOKABLE void setIsMainWindowActive(bool value);

    const MainPageSelection &selectedPage() const { return m_selectedPage; }

    ICallState *callInForeground() const { return m_callInForeground; }
    void setCallInForeground(ICallState *call);
    Q_INVOKABLE void setCallInForeground(const QString &accountId, int callId);

    IChatRoom *selectedChatRoom() const { return m_selectedChatRoom; }
    void setSelectedChatRoom(IChatRoom *chatRoom);

private:
    explicit SelectionState(QObject *parent = nullptr);

    bool m_isMainWindowActive = false;

    MainPageSelection m_selectedPage;
    IChatRoom *m_selectedChatRoom = nullptr;
    QMetaObject::Connection m_selectedChatRoomDestroyedConnection;

    ICallState *m_callInForeground = nullptr;
    QMetaObject::Connection m_callInForegroundDestroyedConnection;

Q_SIGNALS:
    void isMainWindowActiveChanged();
    void selectedPageChanged();
    void selectedChatRoomChanged();
    void callInForegroundChanged();
};

class SelectionStateWrapper
{
    Q_GADGET
    QML_FOREIGN(SelectionState)
    QML_NAMED_ELEMENT(SelectionState)
    QML_SINGLETON

public:
    static SelectionState *create(QQmlEngine *, QJSEngine *) { QQmlEngine::setObjectOwnership(&SelectionState::instance(), QQmlEngine::CppOwnership);
        return &SelectionState::instance(); }

private:
    SelectionStateWrapper() = default;
};
