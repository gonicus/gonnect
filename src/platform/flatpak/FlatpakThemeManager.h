#pragma once
#include <QObject>
#include <QColor>
#include "SettingsPortal.h"
#include "../ThemeManager.h"

class FlatpakThemeManager : public ThemeManager
{
    Q_OBJECT

public:
    explicit FlatpakThemeManager();

    ThemeManager::ColorScheme colorScheme() const override
    {
        return m_settingsPortal->colorScheme();
    }
    QColor accentColor() const override { return m_settingsPortal->accentColor(); }
    bool highContrast() const override { return m_settingsPortal->highContrast(); }

    void shutdown() override { };

private:
    SettingsPortal *m_settingsPortal = nullptr;
};
