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
    Q_PROPERTY(bool useOwnDecoration READ useOwnDecoration NOTIFY useOwnDecorationChanged FINAL)
    Q_PROPERTY(uint fontPixelSize READ fontPixelSize CONSTANT FINAL)
    Q_PROPERTY(uint d READ d CONSTANT FINAL)
    Q_PROPERTY(uint feedbackTimeout READ feedbackTimeout CONSTANT FINAL)

    Q_PROPERTY(QColor primaryTextColor READ primaryTextColor NOTIFY colorPaletteChanged FINAL)
    Q_PROPERTY(QColor whiteColor READ whiteColor NOTIFY colorPaletteChanged FINAL)
    Q_PROPERTY(QColor foregroundHeaderIcons READ foregroundHeaderIcons NOTIFY colorPaletteChanged
                       FINAL)
    Q_PROPERTY(QColor foregroundHeaderIconsInactive READ foregroundHeaderIconsInactive NOTIFY
                       colorPaletteChanged FINAL)
    Q_PROPERTY(QColor foregroundInitials READ foregroundInitials NOTIFY colorPaletteChanged FINAL)
    Q_PROPERTY(QColor secondaryTextColor READ secondaryTextColor NOTIFY colorPaletteChanged FINAL)
    Q_PROPERTY(QColor inactiveTextColor READ inactiveTextColor NOTIFY colorPaletteChanged FINAL)
    Q_PROPERTY(QColor secondaryInactiveTextColor READ secondaryInactiveTextColor NOTIFY
                       colorPaletteChanged FINAL)
    Q_PROPERTY(QColor accentColor READ accentColor NOTIFY accentColorChanged FINAL)
    Q_PROPERTY(QColor borderColor READ borderColor NOTIFY colorPaletteChanged FINAL)
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
    Q_PROPERTY(QColor orangeColor READ orangeColor NOTIFY colorPaletteChanged FINAL)
    Q_PROPERTY(QColor emergencyColor READ emergencyColor NOTIFY colorPaletteChanged FINAL)
    Q_PROPERTY(
            QColor activeIndicatorColor READ activeIndicatorColor NOTIFY colorPaletteChanged FINAL)
    Q_PROPERTY(QColor greenColor READ greenColor NOTIFY colorPaletteChanged FINAL)
    Q_PROPERTY(QColor yellowColor READ yellowColor NOTIFY colorPaletteChanged FINAL)
    Q_PROPERTY(QColor darkGreenColor READ darkGreenColor NOTIFY colorPaletteChanged FINAL)
    Q_PROPERTY(QColor paneColor READ paneColor NOTIFY colorPaletteChanged FINAL)

    Q_PROPERTY(QColor rttBubbleSelf READ rttBubbleSelf NOTIFY rttBubbleSelfChanged FINAL)
    Q_PROPERTY(QColor rttTextSelf READ rttTextSelf NOTIFY rttTextSelfChanged FINAL)
    Q_PROPERTY(QColor rttBubbleOther READ rttBubbleOther NOTIFY rttBubbleOtherChanged FINAL)
    Q_PROPERTY(QColor rttTextOther READ rttTextOther NOTIFY rttTextOtherChanged FINAL)

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
    bool useOwnDecoration();

    /// ms how long a visual feedback shall be visible
    uint feedbackTimeout() { return 3000; }

    Q_INVOKABLE QColor pickForegroundColor(const QColor &backgroundColor) const;
    QColor readableOn(const QColor &background) const;

    // Base colors (seeds) - all other regular colors will be derived from these
    static const QColor seedInkLight;
    static const QColor seedPaperLight;
    static const QColor seedInkDark;
    static const QColor seedVeilBaseDark;
    static const QColor seedAccentLight;
    static const QColor seedAccentDark;
    static const QColor seedBubbleLight;
    static const QColor seedBubbleDark;
    static const QColor seedHighlightDark;
    static const QColor seedInitials;

    // Color helper functions
    static QColor textVeil(const QColor &base, qreal alpha);
    static QColor neutralSurface(qreal lightness);
    static QColor mix(const QColor &a, const QColor &b, qreal t);
    static qreal relativeLuminance(const QColor &color);
    static qreal contrastRatio(const QColor &a, const QColor &b);

    Q_INVOKABLE void setUseOwnDecoration(bool value);

    QColor primaryTextColor() const { return m_primaryTextColor; }
    QColor whiteColor() const { return m_whiteColor; }
    QColor foregroundHeaderIcons() const { return m_foregroundHeaderIcons; }
    QColor foregroundHeaderIconsInactive() const { return m_foregroundHeaderIconsInactive; }
    QColor foregroundInitials() const { return m_foregroundInitials; }
    QColor secondaryTextColor() const { return m_secondaryTextColor; }
    QColor inactiveTextColor() const { return m_inactiveTextColor; }
    QColor secondaryInactiveTextColor() const { return m_secondaryInactiveTextColor; }
    QColor accentColor() const { return m_accentColor; }
    QColor borderColor() const { return m_borderColor; }
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
    QColor orangeColor() const { return m_orangeColor; }
    QColor yellowColor() const { return m_yellowColor; }
    QColor emergencyColor() const { return m_emergencyColor; }
    QColor activeIndicatorColor() const { return m_activeIndicatorColor; }
    QColor greenColor() const { return m_greenColor; }
    QColor darkGreenColor() const { return m_darkGreenColor; }
    QColor paneColor() const { return m_paneColor; }

    QColor rttBubbleSelf() const { return m_rttBubbleSelf; }
    QColor rttTextSelf() const { return m_rttTextSelf; }
    QColor rttBubbleOther() const { return m_rttBubbleOther; }
    QColor rttTextOther() const { return m_rttTextOther; }

private Q_SLOTS:
    void updateColorPalette();
    void updateAccentColor();
    void onThemeVariantChanged();

Q_SIGNALS:
    void themeVariantChanged();
    void isDarkModeChanged();
    void colorPaletteChanged();
    void useOwnDecorationChanged();
    void accentColorChanged();

    void rttBubbleSelfChanged();
    void rttTextSelfChanged();
    void rttBubbleOtherChanged();
    void rttTextOtherChanged();

private:
    void setDarkMode(bool value);
    QColor resolvedSystemAccent() const;
    uint fontPixelSize() const { return 13; }
    uint d() const { return 12; }

    ThemeVariant m_themeVariant = ThemeVariant::System;
    bool m_isDarkMode = false;
    bool m_useOwnDecoration = false;
    bool m_useOwnDecorationInitalized = false;

    QColor m_primaryTextColor;
    QColor m_whiteColor;
    QColor m_foregroundHeaderIcons;
    QColor m_foregroundHeaderIconsInactive;
    QColor m_foregroundInitials;
    QColor m_secondaryTextColor;
    QColor m_inactiveTextColor;
    QColor m_secondaryInactiveTextColor;
    QColor m_accentColor;
    QColor m_borderColor;
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
    QColor m_orangeColor;
    QColor m_yellowColor;
    QColor m_emergencyColor;
    QColor m_greenColor;
    QColor m_darkGreenColor;
    QColor m_paneColor;
    QColor m_activeIndicatorColor;

    QColor m_rttBubbleSelf;
    QColor m_rttTextSelf;
    QColor m_rttBubbleOther;
    QColor m_rttTextOther;
};

class ThemeWrapper
{
    Q_GADGET
    QML_FOREIGN(Theme)
    QML_NAMED_ELEMENT(Theme)
    QML_SINGLETON

public:
    static Theme *create(QQmlEngine *, QJSEngine *)
    {
        QQmlEngine::setObjectOwnership(&Theme::instance(), QQmlEngine::CppOwnership);
        return &Theme::instance();
    }

private:
    ThemeWrapper() = default;
};
