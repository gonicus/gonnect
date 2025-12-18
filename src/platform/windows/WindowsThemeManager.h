#pragma once
#include <QObject>
#include <QColor>
#include <QSettings>
#include <windows.h>
#include <QTHread>
#include <QSharedPointer>
#include <QtCore/private/quniquehandle_types_p.h>
#include "../ThemeManager.h"

class WindowsThemeManager : public ThemeManager
{
    Q_OBJECT

public:
    explicit WindowsThemeManager();

    QColor accentColor() const override { return QColor("blue"); }
    bool highContrast() const override { return false; }

    void shutdown() override { }

protected:
    void initColorThemeDetection() override;

private:
    void updateColorTheme();
    void startThemeWatcher();

    QSettings *m_themeSettings = nullptr;
    HKEY m_key = nullptr;
    QUniqueWin32NullHandle m_keyChangedEvent;
};
