#pragma once

#include <QObject>
#include <QQmlEngine>

#include "MainPageSelection.h"

class SelectionState : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(SelectionState)

    Q_PROPERTY(
            MainPageSelection selectedPage MEMBER m_selectedPage NOTIFY selectedPageChanged FINAL)

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

private:
    explicit SelectionState(QObject *parent = nullptr);

    MainPageSelection m_selectedPage;

Q_SIGNALS:
    void selectedPageChanged();
};

class SelectionStateWrapper
{
    Q_GADGET
    QML_FOREIGN(SelectionState)
    QML_NAMED_ELEMENT(SelectionState)
    QML_SINGLETON

public:
    static SelectionState *create(QQmlEngine *, QJSEngine *) { return &SelectionState::instance(); }

private:
    SelectionStateWrapper() = default;
};
