#pragma once
#include <QObject>
#include "../GlobalShortcuts.h"

class DummyGlobalShortcuts : public GlobalShortcuts
{
    Q_OBJECT

public:
    explicit DummyGlobalShortcuts() : GlobalShortcuts() {}

    bool isSupported() const override { return false; }
    void setShortcuts(Shortcuts &shortcuts) override { Q_UNUSED(shortcuts); }
    QList<ShortcutItem *> shortcuts() const override { return QList<ShortcutItem*>(); }
};
