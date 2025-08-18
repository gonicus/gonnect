#include "Theme.h"
#include "SettingsPortal.h"
#include "AppSettings.h"

#include <QLoggingCategory>
#include <QRegularExpression>
#include <private/qzipreader_p.h>

Q_LOGGING_CATEGORY(lcTheme, "gonnect.app.theme")

Theme::Theme(QObject *parent) : QObject{ parent }
{

    // Setup theme variant
    connect(this, &Theme::themeVariantChanged, this, &Theme::onThemeVariantChanged);
    connect(&SettingsPortal::instance(), &SettingsPortal::colorSchemeChanged, this,
            &Theme::onThemeVariantChanged);

    AppSettings settings;
    m_themeVariant = static_cast<ThemeVariant>(settings.value("generic/themeVariant", 0).toUInt());

    onThemeVariantChanged();

    // Setup listeners for dark mode
    connect(this, &Theme::isDarkModeChanged, this, &Theme::updateColorPalette);

    updateColorPalette();
    useOwnDecoration();
}

bool Theme::useOwnDecoration()
{
    if (!m_useOwnDecorationInitalized) {
        m_useOwnDecorationInitalized = true;

        AppSettings settings;
        const auto settingsVal =
                settings.value("generic/useOwnWindowDecoration", "true").toString();

        if (settingsVal == "auto") {
            const auto desktop =
                    QString::fromLocal8Bit(qgetenv("XDG_SESSION_DESKTOP")).toLower(); // gnome|kde
            m_useOwnDecoration = desktop == "gnome";
        } else {
            m_useOwnDecoration = settingsVal == "true";
        }
    }
    return m_useOwnDecoration;
}

void Theme::setUseOwnDecoration(bool value)
{
    if (m_useOwnDecoration != value) {
        m_useOwnDecoration = value;
        AppSettings settings;
        settings.setValue("generic/useOwnWindowDecoration", value ? "true" : "false");
        emit useOwnDecorationChanged();
    }
}

void Theme::onThemeVariantChanged()
{
    AppSettings settings;
    settings.setValue("generic/themeVariant", static_cast<uint>(m_themeVariant));

    switch (m_themeVariant) {

    case ThemeVariant::System: {
        const auto portalScheme = SettingsPortal::instance().colorScheme();
        switch (portalScheme) {
        case SettingsPortal::ColorScheme::NO_PREFERENCE:
            setDarkMode(false);
            break;

        case SettingsPortal::ColorScheme::DARK:
            setDarkMode(true);
            break;

        case SettingsPortal::ColorScheme::LIGHT:
            setDarkMode(false);
            break;
        }
        break;
    }

    case ThemeVariant::Light:
        setDarkMode(false);
        break;

    case ThemeVariant::Dark:
        setDarkMode(true);
        break;
    }
}

void Theme::updateColorPalette()
{
    m_primaryTextColor = QColor(5, 5, 5);
    m_foregroundWhiteColor = QColor(255, 255, 255);
    m_foregroundHeaderIcons = QColor(46, 52, 54);
    m_foregroundHeaderIconsInactive = QColor(125, 129, 130);
    m_foregroundInitials = QColor(40, 34, 80);
    m_secondaryTextColor = QColor(153, 153, 153);
    m_inactiveTextColor = QColor(104, 104, 104);
    m_secondaryInactiveTextColor = QColor(168, 168, 168);
    m_accentColor = QColor(30, 57, 143);
    m_borderColor = QColor(219, 219, 219);
    m_borderHeaderIconHovered = QColor(206, 201, 196);
    m_highlightColor = QColor(30, 57, 143, 76);
    m_paneColor = QColor(246, 245, 244);
    m_backgroundColor = QColor(255, 255, 255);
    m_backgroundSecondaryColor = QColor(250, 250, 250);
    m_backgroundOffsetColor = QColor(0, 0, 0, 20);
    m_backgroundOffsetHoveredColor = QColor(0, 0, 0, 40);
    m_backgroundHeader = QColor(235, 235, 235);
    m_backgroundHeaderSelected = QColor(216, 216, 216);
    m_backgroundHeaderInactive = QColor(242, 242, 242);
    m_backgroundHeaderIconHovered = QColor(248, 248, 247);
    m_backgroundInitials = QColor(214, 212, 233);
    m_shadowColor = QColor(0, 0, 0, 32);
    m_redColor = QColor(224, 27, 36);
    m_greenColor = QColor(36, 181, 27);
    m_darkGreenColor = QColor(128, 128, 0);

    // Dark mode overrides
    if (m_isDarkMode) {
        m_primaryTextColor = QColor(248, 248, 248);
        m_secondaryTextColor = QColor(190, 190, 190);
        m_foregroundHeaderIcons = QColor(238, 238, 236);
        m_foregroundHeaderIconsInactive = QColor(157, 157, 156);
        m_secondaryInactiveTextColor = QColor(108, 108, 108);
        m_backgroundColor = QColor(53, 53, 53);
        m_borderColor = QColor(33, 33, 33);
        m_borderHeaderIconHovered = QColor(28, 28, 28);
        m_backgroundSecondaryColor = QColor(70, 70, 70);
        m_backgroundOffsetColor = QColor(230, 230, 230, 20);
        m_backgroundOffsetHoveredColor = QColor(230, 230, 230, 40);
        m_backgroundHeader = QColor(48, 48, 48);
        m_backgroundHeaderInactive = QColor(36, 36, 36);
        m_backgroundHeaderIconHovered = QColor(55, 55, 55);
        m_accentColor = QColor(255, 255, 255, 120);
        m_highlightColor = QColor(15, 83, 158, 36);
        m_paneColor = QColor(45, 45, 45);
    }

    emit colorPaletteChanged();
}

void Theme::setDarkMode(bool value)
{
    if (m_isDarkMode != value) {
        m_isDarkMode = value;
        emit isDarkModeChanged();
    }
}

QString Theme::toCamelCase(const QString &str) const
{
    QStringList parts = str.split(QChar('_'));
    for (int i = 1; i < parts.size(); ++i) {
        parts[i].replace(0, 1, parts[i][0].toUpper());
    }
    return parts.join("");
}
