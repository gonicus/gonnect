#pragma once
#include <QObject>
#include <QLinkedList>
#include "../GlobalShortcuts.h"

class QHotkey;

class WindowsGlobalShortcuts : public GlobalShortcuts
{
    Q_OBJECT

public:
    explicit WindowsGlobalShortcuts();
    ~WindowsGlobalShortcuts();

    bool isSupported() const override { return true; }
    void setShortcuts(Shortcuts &shortcuts) override;
    QList<ShortcutItem *> shortcuts() const override { return m_currentShortcuts; }

private:
    bool m_enabled = false;
    QList<ShortcutItem *> m_currentShortcuts;
    QLinkedList<QHotkey *> m_hotkeys;
};
