#include "WindowsGlobalShortcuts.h"
#include "ReadOnlyConfdSettings.h"
#include <QHotkey>

Q_LOGGING_CATEGORY(lcShortcuts, "gonnect.session.shortcuts")

GlobalShortcuts &GlobalShortcuts::instance()
{
    static GlobalShortcuts *_instance = nullptr;
    if (!_instance) {
        _instance = new WindowsGlobalShortcuts;
    }
    return *_instance;
}

WindowsGlobalShortcuts::WindowsGlobalShortcuts() : GlobalShortcuts{ }
{
    ReadOnlyConfdSettings settings;
    m_enabled = !settings.value("generic/disableGlobalShortcuts", false).toBool();
}

WindowsGlobalShortcuts::~WindowsGlobalShortcuts()
{
    qDeleteAll(m_currentShortcuts);
    m_currentShortcuts.clear();
    qDeleteAll(m_hotkeys);
    m_hotkeys.clear();
}

void WindowsGlobalShortcuts::setShortcuts(Shortcuts &shortcuts)
{
    qDeleteAll(m_currentShortcuts);
    m_currentShortcuts.clear();
    qDeleteAll(m_hotkeys);
    m_hotkeys.clear();
    if (m_enabled) {
        ReadOnlyConfdSettings settings;
        for (auto &sc : std::as_const(shortcuts)) {
            auto trigger = sc.second.value("preferred_trigger", QString()).toString();
            QString id = sc.first;
            if (trigger.isEmpty()) {
                qCWarning(lcShortcuts) << "invalid preferred_trigger for shortcut" << id;
                continue;
            }
            trigger = settings.value(QString("windows_shortcuts/%1").arg(id), trigger).toString();

            QKeySequence ks = QKeySequence::fromString(trigger);
            if (ks.isEmpty()) {
                qCInfo(lcShortcuts) << "no trigger for shortcut" << id;
            } else {
                auto *h = new QHotkey(ks, true, this);
                if (h->isRegistered()) {
                    qCInfo(lcShortcuts) << "registered shortcut" << id << "with" << trigger;
                    connect(h, &QHotkey::activated, this, [this, id]() { Q_EMIT activated(id); });
                    m_hotkeys.append(h);
                } else {
                    qCInfo(lcShortcuts) << "failed to register shortcut" << id << "with" << trigger;
                    delete h;
                }
            }

            auto sci = new ShortcutItem();
            sci->id = id;
            sci->description = sc.second.value("description", "Error: no description").toString();
            sci->triggerDescription = trigger;
            m_currentShortcuts.push_back(sci);
        }
    }

    Q_EMIT initialized();
}
