#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QColor>

class Theme : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("RegisterEnumClassesUnscoped", "false")

    Q_PROPERTY(
            Theme::ThemeVariant themeVariant MEMBER m_themeVariant NOTIFY themeVariantChanged FINAL)
    Q_PROPERTY(bool isDarkMode READ isDarkMode NOTIFY isDarkModeChanged FINAL)
    Q_PROPERTY(bool useOwnDecoration READ useOwnDecoration CONSTANT FINAL)

    Q_PROPERTY(QColor primaryTextColor READ primaryTextColor NOTIFY colorPaletteChanged FINAL)
    Q_PROPERTY(
            QColor foregroundWhiteColor READ foregroundWhiteColor NOTIFY colorPaletteChanged FINAL)
    Q_PROPERTY(QColor foregroundHeaderIcons READ foregroundHeaderIcons NOTIFY colorPaletteChanged
                       FINAL)
    Q_PROPERTY(QColor foregroundHeaderIconsInactive READ foregroundHeaderIconsInactive NOTIFY
                       colorPaletteChanged FINAL)
    Q_PROPERTY(QColor foregroundInitials READ foregroundInitials NOTIFY colorPaletteChanged FINAL)
    Q_PROPERTY(QColor secondaryTextColor READ secondaryTextColor NOTIFY colorPaletteChanged FINAL)
    Q_PROPERTY(QColor accentColor READ accentColor NOTIFY colorPaletteChanged FINAL)
    Q_PROPERTY(QColor borderColor READ borderColor NOTIFY colorPaletteChanged FINAL)
    Q_PROPERTY(QColor borderHeaderIconHovered READ borderHeaderIconHovered NOTIFY
                       colorPaletteChanged FINAL)
    Q_PROPERTY(QColor highlightColor READ highlightColor NOTIFY colorPaletteChanged FINAL)
    Q_PROPERTY(QColor backgroundColor READ backgroundColor NOTIFY colorPaletteChanged FINAL)
    Q_PROPERTY(QColor backgroundSecondaryColor READ backgroundSecondaryColor NOTIFY
                       colorPaletteChanged FINAL)
    Q_PROPERTY(QColor backgroundOffsetColor READ backgroundOffsetColor NOTIFY colorPaletteChanged
                       FINAL)
    Q_PROPERTY(QColor backgroundOffsetHoveredColor READ backgroundOffsetHoveredColor NOTIFY
                       colorPaletteChanged FINAL)
    Q_PROPERTY(QColor backgroundHeader READ backgroundHeader NOTIFY colorPaletteChanged FINAL)
    Q_PROPERTY(QColor backgroundHeaderInactive READ backgroundHeaderInactive NOTIFY
                       colorPaletteChanged FINAL)
    Q_PROPERTY(QColor backgroundHeaderIconHovered READ backgroundHeaderIconHovered NOTIFY
                       colorPaletteChanged FINAL)
    Q_PROPERTY(QColor backgroundInitials READ backgroundInitials NOTIFY colorPaletteChanged FINAL)
    Q_PROPERTY(QColor shadowColor READ shadowColor NOTIFY colorPaletteChanged FINAL)
    Q_PROPERTY(QColor redColor READ redColor NOTIFY colorPaletteChanged FINAL)
    Q_PROPERTY(QColor greenColor READ greenColor NOTIFY colorPaletteChanged FINAL)
    Q_PROPERTY(QColor darkGreenColor READ darkGreenColor NOTIFY colorPaletteChanged FINAL)
    Q_PROPERTY(QColor paneColor READ paneColor NOTIFY colorPaletteChanged FINAL)

public:
    static Theme &instance()
    {
        static Theme *_instance = nullptr;
        if (!_instance) {
            _instance = new Theme;
        }
        return *_instance;
    };

    enum class ThemeVariant { System, Light, Dark };
    Q_ENUM(ThemeVariant)

    explicit Theme(QObject *parent = nullptr);

    bool isDarkMode() const { return m_isDarkMode; }
    bool useOwnDecoration() const;

    QColor primaryTextColor() const { return m_primaryTextColor; }
    QColor foregroundWhiteColor() const { return m_foregroundWhiteColor; }
    QColor foregroundHeaderIcons() const { return m_foregroundHeaderIcons; }
    QColor foregroundHeaderIconsInactive() const { return m_foregroundHeaderIconsInactive; }
    QColor foregroundInitials() const { return m_foregroundInitials; }
    QColor secondaryTextColor() const { return m_secondaryTextColor; }
    QColor accentColor() const { return m_accentColor; }
    QColor borderColor() const { return m_borderColor; }
    QColor borderHeaderIconHovered() const { return m_borderHeaderIconHovered; }
    QColor highlightColor() const { return m_highlightColor; }
    QColor backgroundColor() const { return m_backgroundColor; }
    QColor backgroundSecondaryColor() const { return m_backgroundSecondaryColor; }
    QColor backgroundOffsetColor() const { return m_backgroundOffsetColor; }
    QColor backgroundOffsetHoveredColor() const { return m_backgroundOffsetHoveredColor; }
    QColor backgroundHeader() const { return m_backgroundHeader; }
    QColor backgroundHeaderInactive() const { return m_backgroundHeaderInactive; }
    QColor backgroundHeaderIconHovered() const { return m_backgroundHeaderIconHovered; }
    QColor backgroundInitials() const { return m_backgroundInitials; }
    QColor shadowColor() const { return m_shadowColor; }
    QColor redColor() const { return m_redColor; }
    QColor greenColor() const { return m_greenColor; }
    QColor darkGreenColor() const { return m_darkGreenColor; }
    QColor paneColor() const { return m_paneColor; }

private slots:
    void updateColorPalette();
    void onThemeVariantChanged();

signals:
    void themeVariantChanged();
    void isDarkModeChanged();
    void colorPaletteChanged();

private:
    void setDarkMode(bool value);

    ThemeVariant m_themeVariant = ThemeVariant::System;
    bool m_isDarkMode = false;

    QColor m_primaryTextColor;
    QColor m_foregroundWhiteColor;
    QColor m_foregroundHeaderIcons;
    QColor m_foregroundHeaderIconsInactive;
    QColor m_foregroundInitials;
    QColor m_secondaryTextColor;
    QColor m_accentColor;
    QColor m_borderColor;
    QColor m_borderHeaderIconHovered;
    QColor m_highlightColor;
    QColor m_backgroundColor;
    QColor m_backgroundSecondaryColor;
    QColor m_backgroundOffsetColor;
    QColor m_backgroundOffsetHoveredColor;
    QColor m_backgroundHeader;
    QColor m_backgroundHeaderInactive;
    QColor m_backgroundHeaderIconHovered;
    QColor m_backgroundInitials;
    QColor m_shadowColor;
    QColor m_redColor;
    QColor m_greenColor;
    QColor m_darkGreenColor;
    QColor m_paneColor;
};

class ThemeWrapper
{
    Q_GADGET
    QML_FOREIGN(Theme)
    QML_NAMED_ELEMENT(Theme)
    QML_SINGLETON

public:
    static Theme *create(QQmlEngine *, QJSEngine *) { return &Theme::instance(); }

private:
    ThemeWrapper() = default;
};
