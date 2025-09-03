#include "FlatpakGlobalShortcuts.h"
#include "GlobalShortcuts.h"
#include "GlobalShortcutPortal.h"

GlobalShortcuts &GlobalShortcuts::instance()
{
    static GlobalShortcuts *_instance = nullptr;
    if (!_instance) {
        _instance = new FlatpakGlobalShortcuts;
    }
    return *_instance;
}

FlatpakGlobalShortcuts::FlatpakGlobalShortcuts() : GlobalShortcuts{}
{
    m_portal = new GlobalShortcutPortal(this);

    connect(m_portal, &GlobalShortcutPortal::initialized, this,
            &FlatpakGlobalShortcuts::initialized);
    connect(m_portal, &GlobalShortcutPortal::shortcutsChanged, this,
            &FlatpakGlobalShortcuts::shortcutsChanged);
    connect(m_portal, &GlobalShortcutPortal::activated, this,
            [this](const QString &id) { emit activated(id); });
}

bool FlatpakGlobalShortcuts::isSupported() const
{
    return m_portal->isSupported();
}

void FlatpakGlobalShortcuts::setShortcuts(Shortcuts &shortcuts)
{
    m_portal->setShortcuts(shortcuts);
    m_portal->initialize();
}

QList<ShortcutItem *> FlatpakGlobalShortcuts::shortcuts() const
{
    return m_portal->shortcuts();
}
