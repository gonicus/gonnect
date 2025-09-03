#pragma once

#include <QObject>

class ThemeManager : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(ThemeManager)

public:
    Q_REQUIRED_RESULT static ThemeManager &instance();

    explicit ThemeManager() : QObject() { }
    ~ThemeManager() = default;

    enum class ColorScheme {
        NO_PREFERENCE,
        DARK,
        LIGHT,
    };
    Q_ENUM(ColorScheme)

    virtual ColorScheme colorScheme() const = 0;
    virtual QColor accentColor() const = 0;
    virtual bool highContrast() const = 0;

    virtual void shutdown() = 0;

signals:
    void colorSchemeChanged();
    void accentColorChanged();
    void highContrastChanged();
};
