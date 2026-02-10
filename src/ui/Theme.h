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

    Q_PROPERTY(QColor primaryTextColor READ primaryTextColor NOTIFY colorPaletteChanged FINAL)
    Q_PROPERTY(
            QColor foregroundWhiteColor READ foregroundWhiteColor NOTIFY colorPaletteChanged FINAL)
    Q_PROPERTY(QColor foregroundHeaderIcons READ foregroundHeaderIcons NOTIFY colorPaletteChanged
                       FINAL)
    Q_PROPERTY(QColor foregroundHeaderIconsInactive READ foregroundHeaderIconsInactive NOTIFY
                       colorPaletteChanged FINAL)
    Q_PROPERTY(QColor foregroundInitials READ foregroundInitials NOTIFY colorPaletteChanged FINAL)
    Q_PROPERTY(QColor secondaryTextColor READ secondaryTextColor NOTIFY colorPaletteChanged FINAL)
    Q_PROPERTY(QColor inactiveTextColor READ inactiveTextColor NOTIFY colorPaletteChanged FINAL)
    Q_PROPERTY(QColor secondaryInactiveTextColor READ secondaryInactiveTextColor NOTIFY
                       colorPaletteChanged FINAL)
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
    Q_PROPERTY(QColor backgroundHeaderSelected READ backgroundHeaderSelected NOTIFY
                       colorPaletteChanged FINAL)
    Q_PROPERTY(QColor backgroundHeaderInactive READ backgroundHeaderInactive NOTIFY
                       colorPaletteChanged FINAL)
    Q_PROPERTY(QColor backgroundHeaderIconHovered READ backgroundHeaderIconHovered NOTIFY
                       colorPaletteChanged FINAL)
    Q_PROPERTY(QColor backgroundInitials READ backgroundInitials NOTIFY colorPaletteChanged FINAL)
    Q_PROPERTY(QColor shadowColor READ shadowColor NOTIFY colorPaletteChanged FINAL)
    Q_PROPERTY(QColor redColor READ redColor NOTIFY colorPaletteChanged FINAL)
    Q_PROPERTY(
            QColor activeIndicatorColor READ activeIndicatorColor NOTIFY colorPaletteChanged FINAL)
    Q_PROPERTY(QColor greenColor READ greenColor NOTIFY colorPaletteChanged FINAL)
    Q_PROPERTY(QColor darkGreenColor READ darkGreenColor NOTIFY colorPaletteChanged FINAL)
    Q_PROPERTY(QColor paneColor READ paneColor NOTIFY colorPaletteChanged FINAL)
    Q_PROPERTY(QColor highContrastColor READ highContrastColor NOTIFY colorPaletteChanged FINAL)

    Q_PROPERTY(QColor buttonBackgroundHover READ buttonBackgroundHover NOTIFY
                       buttonBackgroundHoverChanged FINAL)
    Q_PROPERTY(QColor frame READ frame NOTIFY frameChanged FINAL)
    Q_PROPERTY(QColor icons READ icons NOTIFY iconsChanged FINAL)
    Q_PROPERTY(QColor iconsAttention READ iconsAttention NOTIFY iconsAttentionChanged FINAL)
    Q_PROPERTY(QColor ntpBackground READ ntpBackground NOTIFY ntpBackgroundChanged FINAL)
    Q_PROPERTY(QColor popup READ popup NOTIFY popupChanged FINAL)
    Q_PROPERTY(QColor popupBorder READ popupBorder NOTIFY popupBorderChanged FINAL)
    Q_PROPERTY(QColor popupText READ popupText NOTIFY popupTextChanged FINAL)
    Q_PROPERTY(QColor sidebar READ sidebar NOTIFY sidebarChanged FINAL)
    Q_PROPERTY(QColor sidebarBorder READ sidebarBorder NOTIFY sidebarBorderChanged FINAL)
    Q_PROPERTY(QColor sidebarText READ sidebarText NOTIFY sidebarTextChanged FINAL)
    Q_PROPERTY(
            QColor tabBackgroundText READ tabBackgroundText NOTIFY tabBackgroundTextChanged FINAL)
    Q_PROPERTY(QColor tabLine READ tabLine NOTIFY tabLineChanged FINAL)
    Q_PROPERTY(QColor tabLoading READ tabLoading NOTIFY tabLoadingChanged FINAL)
    Q_PROPERTY(QColor tabText READ tabText NOTIFY tabTextChanged FINAL)
    Q_PROPERTY(QColor toolbar READ toolbar NOTIFY toolbarChanged FINAL)
    Q_PROPERTY(QColor toolbarBottomSeparator READ toolbarBottomSeparator NOTIFY
                       toolbarBottomSeparatorChanged FINAL)
    Q_PROPERTY(QColor toolbarField READ toolbarField NOTIFY toolbarFieldChanged FINAL)
    Q_PROPERTY(QColor toolbarFieldBorder READ toolbarFieldBorder NOTIFY toolbarFieldBorderChanged
                       FINAL)
    Q_PROPERTY(QColor toolbarFieldBorderFocus READ toolbarFieldBorderFocus NOTIFY
                       toolbarFieldBorderFocusChanged FINAL)
    Q_PROPERTY(QColor toolbarFieldText READ toolbarFieldText NOTIFY toolbarFieldTextChanged FINAL)
    Q_PROPERTY(QColor toolbarFieldTextFocus READ toolbarFieldTextFocus NOTIFY
                       toolbarFieldTextFocusChanged FINAL)
    Q_PROPERTY(QColor toolbarText READ toolbarText NOTIFY toolbarTextChanged FINAL)
    Q_PROPERTY(QColor toolbarTopSeparator READ toolbarTopSeparator NOTIFY toolbarTopSeparatorChanged
                       FINAL)

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

    Q_INVOKABLE void setUseOwnDecoration(bool value);

    QColor primaryTextColor() const { return m_primaryTextColor; }
    QColor foregroundWhiteColor() const { return m_foregroundWhiteColor; }
    QColor foregroundHeaderIcons() const { return m_foregroundHeaderIcons; }
    QColor foregroundHeaderIconsInactive() const { return m_foregroundHeaderIconsInactive; }
    QColor foregroundInitials() const { return m_foregroundInitials; }
    QColor secondaryTextColor() const { return m_secondaryTextColor; }
    QColor inactiveTextColor() const { return m_inactiveTextColor; }
    QColor secondaryInactiveTextColor() const { return m_secondaryInactiveTextColor; }
    QColor accentColor() const { return m_accentColor; }
    QColor borderColor() const { return m_borderColor; }
    QColor borderHeaderIconHovered() const { return m_borderHeaderIconHovered; }
    QColor highlightColor() const { return m_highlightColor; }
    QColor backgroundColor() const { return m_backgroundColor; }
    QColor backgroundSecondaryColor() const { return m_backgroundSecondaryColor; }
    QColor backgroundOffsetColor() const { return m_backgroundOffsetColor; }
    QColor backgroundOffsetHoveredColor() const { return m_backgroundOffsetHoveredColor; }
    QColor backgroundHeader() const { return m_backgroundHeader; }
    QColor backgroundHeaderSelected() const { return m_backgroundHeaderSelected; }
    QColor backgroundHeaderInactive() const { return m_backgroundHeaderInactive; }
    QColor backgroundHeaderIconHovered() const { return m_backgroundHeaderIconHovered; }
    QColor backgroundInitials() const { return m_backgroundInitials; }
    QColor shadowColor() const { return m_shadowColor; }
    QColor redColor() const { return m_redColor; }
    QColor activeIndicatorColor() const { return m_activeIndicatorColor; }
    QColor greenColor() const { return m_greenColor; }
    QColor darkGreenColor() const { return m_darkGreenColor; }
    QColor paneColor() const { return m_paneColor; }
    QColor highContrastColor() const { return m_highContrastColor; }

    QColor buttonBackgroundHover() const { return m_buttonBackgroundHover; }
    QColor frame() const { return m_frame; }
    QColor icons() const { return m_icons; }
    QColor iconsAttention() const { return m_iconsAttention; }
    QColor ntpBackground() const { return m_ntpBackground; }
    QColor popup() const { return m_popup; }
    QColor popupBorder() const { return m_popupBorder; }
    QColor popupText() const { return m_popupText; }
    QColor sidebar() const { return m_sidebar; }
    QColor sidebarBorder() const { return m_sidebarBorder; }
    QColor sidebarText() const { return m_sidebarText; }
    QColor tabBackgroundText() const { return m_tabBackgroundText; }
    QColor tabLine() const { return m_tabLine; }
    QColor tabLoading() const { return m_tabLoading; }
    QColor tabText() const { return m_tabText; }
    QColor toolbar() const { return m_toolbar; }
    QColor toolbarBottomSeparator() const { return m_toolbarBottomSeparator; }
    QColor toolbarField() const { return m_toolbarField; }
    QColor toolbarFieldBorder() const { return m_toolbarFieldBorder; }
    QColor toolbarFieldBorderFocus() const { return m_toolbarFieldBorderFocus; }
    QColor toolbarFieldText() const { return m_toolbarFieldText; }
    QColor toolbarFieldTextFocus() const { return m_toolbarFieldTextFocus; }
    QColor toolbarText() const { return m_toolbarText; }
    QColor toolbarTopSeparator() const { return m_toolbarTopSeparator; }

private Q_SLOTS:
    void updateColorPalette();
    void onThemeVariantChanged();

Q_SIGNALS:
    void themeVariantChanged();
    void isDarkModeChanged();
    void colorPaletteChanged();
    void useOwnDecorationChanged();

    void buttonBackgroundHoverChanged();
    void frameChanged();
    void iconsChanged();
    void iconsAttentionChanged();
    void ntpBackgroundChanged();
    void popupChanged();
    void popupBorderChanged();
    void popupTextChanged();
    void sidebarChanged();
    void sidebarBorderChanged();
    void sidebarTextChanged();
    void tabBackgroundTextChanged();
    void tabLineChanged();
    void tabLoadingChanged();
    void tabTextChanged();
    void toolbarChanged();
    void toolbarBottomSeparatorChanged();
    void toolbarFieldChanged();
    void toolbarFieldBorderChanged();
    void toolbarFieldBorderFocusChanged();
    void toolbarFieldTextChanged();
    void toolbarFieldTextFocusChanged();
    void toolbarTextChanged();
    void toolbarTopSeparatorChanged();

private:
    void setDarkMode(bool value);
    QString toCamelCase(const QString &str) const;

    ThemeVariant m_themeVariant = ThemeVariant::System;
    bool m_isDarkMode = false;
    bool m_useOwnDecoration = false;
    bool m_useOwnDecorationInitalized = false;

    QColor m_primaryTextColor;
    QColor m_foregroundWhiteColor;
    QColor m_foregroundHeaderIcons;
    QColor m_foregroundHeaderIconsInactive;
    QColor m_foregroundInitials;
    QColor m_secondaryTextColor;
    QColor m_inactiveTextColor;
    QColor m_secondaryInactiveTextColor;
    QColor m_accentColor;
    QColor m_borderColor;
    QColor m_borderHeaderIconHovered;
    QColor m_highlightColor;
    QColor m_backgroundColor;
    QColor m_backgroundSecondaryColor;
    QColor m_backgroundOffsetColor;
    QColor m_backgroundOffsetHoveredColor;
    QColor m_backgroundHeader;
    QColor m_backgroundHeaderSelected;
    QColor m_backgroundHeaderInactive;
    QColor m_backgroundHeaderIconHovered;
    QColor m_backgroundInitials;
    QColor m_shadowColor;
    QColor m_redColor;
    QColor m_greenColor;
    QColor m_darkGreenColor;
    QColor m_paneColor;
    QColor m_highContrastColor;
    QColor m_activeIndicatorColor;

    QColor m_buttonBackgroundHover;
    QColor m_frame;
    QColor m_icons;
    QColor m_iconsAttention;
    QColor m_ntpBackground;
    QColor m_popup;
    QColor m_popupBorder;
    QColor m_popupText;
    QColor m_sidebar;
    QColor m_sidebarBorder;
    QColor m_sidebarText;
    QColor m_tabBackgroundText;
    QColor m_tabLine;
    QColor m_tabLoading;
    QColor m_tabText;
    QColor m_toolbar;
    QColor m_toolbarBottomSeparator;
    QColor m_toolbarField;
    QColor m_toolbarFieldBorder;
    QColor m_toolbarFieldBorderFocus;
    QColor m_toolbarFieldText;
    QColor m_toolbarFieldTextFocus;
    QColor m_toolbarText;
    QColor m_toolbarTopSeparator;
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
