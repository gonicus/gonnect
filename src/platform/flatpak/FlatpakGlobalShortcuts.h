#pragma once
#include <QObject>
#include "../GlobalShortcuts.h"

class GlobalShortcutPortal;
;

class FlatpakGlobalShortcuts : public GlobalShortcuts
{
    Q_OBJECT

public:
    explicit FlatpakGlobalShortcuts();

    bool isSupported() const override;
    void setShortcuts(Shortcuts &shortcuts) override;
    QList<ShortcutItem *> shortcuts() const override;

private:
    GlobalShortcutPortal *m_portal = nullptr;
};
