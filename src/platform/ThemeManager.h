#pragma once

#include <QObject>

class ThemeManager : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(ThemeManager)

public:
    Q_REQUIRED_RESULT static ThemeManager &instance();

    explicit ThemeManager();
    ~ThemeManager() = default;

    enum class ColorScheme {
        NO_PREFERENCE,
        DARK,
        LIGHT,
    };
    Q_ENUM(ColorScheme)

    virtual ColorScheme colorScheme() const { return m_colorScheme; }
    virtual ColorScheme trayColorScheme() const;
    virtual QColor accentColor() const = 0;
    virtual bool highContrast() const = 0;

    virtual void shutdown() = 0;

protected:
    virtual void initColorThemeDetection();
    void handleColorSchemeChange(Qt::ColorScheme colorScheme);
    ThemeManager::ColorScheme m_colorScheme = ThemeManager::ColorScheme::NO_PREFERENCE;
    ThemeManager::ColorScheme m_trayColorScheme = ThemeManager::ColorScheme::NO_PREFERENCE;

Q_SIGNALS:
    void colorSchemeChanged();
    void trayColorSchemeChanged();
    void accentColorChanged();
    void highContrastChanged();
};
