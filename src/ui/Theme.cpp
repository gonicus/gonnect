#include "Theme.h"
#include "ThemeManager.h"
#include "AppSettings.h"

#include <QLoggingCategory>
#include <QRegularExpression>

#include <algorithm>
#include <cmath>

Q_LOGGING_CATEGORY(lcTheme, "gonnect.app.theme")

const QColor Theme::seedInkLight = QColor(5, 5, 5);
const QColor Theme::seedPaperLight = QColor(255, 255, 255);
const QColor Theme::seedInkDark = QColor(248, 248, 248);
const QColor Theme::seedVeilBaseDark = QColor(230, 230, 230);
const QColor Theme::seedAccentLight = QColor(30, 57, 143);
const QColor Theme::seedAccentDark = QColor(255, 255, 255, 120);
const QColor Theme::seedBubbleLight = QColor(45, 92, 229);
const QColor Theme::seedBubbleDark = QColor(50, 96, 230);
const QColor Theme::seedHighlightDark = QColor(15, 83, 158);
const QColor Theme::seedInitials = QColor(40, 34, 80);

QColor Theme::textVeil(const QColor &base, qreal alpha)
{
    QColor c = base;
    c.setAlphaF(static_cast<float>(std::clamp(alpha, 0.0, 1.0)) * (base.alphaF()));
    return c;
}

QColor Theme::neutralSurface(qreal lightness)
{
    return QColor::fromHslF(0.0, 0.0, static_cast<float>(std::clamp(lightness, 0.0, 1.0)));
}

QColor Theme::mix(const QColor &a, const QColor &b, qreal t)
{
    t = std::clamp(t, 0.0, 1.0);
    return QColor::fromRgbF(static_cast<float>(a.redF() + (b.redF() - a.redF()) * t),
                            static_cast<float>(a.greenF() + (b.greenF() - a.greenF()) * t),
                            static_cast<float>(a.blueF() + (b.blueF() - a.blueF()) * t),
                            static_cast<float>(a.alphaF() + (b.alphaF() - a.alphaF()) * t));
}

qreal Theme::relativeLuminance(const QColor &color)
{
    auto linearize = [](qreal v) {
        return v <= 0.03928 ? v / 12.92 : std::pow((v + 0.055) / 1.055, 2.4);
    };
    const qreal r = linearize(color.redF());
    const qreal g = linearize(color.greenF());
    const qreal b = linearize(color.blueF());
    return 0.2126 * r + 0.7152 * g + 0.0722 * b;
}

qreal Theme::contrastRatio(const QColor &a, const QColor &b)
{
    const qreal la = relativeLuminance(a);
    const qreal lb = relativeLuminance(b);
    const qreal hi = std::max(la, lb);
    const qreal lo = std::min(la, lb);
    return (hi + 0.05) / (lo + 0.05);
}

Theme::Theme(QObject *parent) : QObject{ parent }
{

    // Setup theme variant
    auto &themeManager = ThemeManager::instance();
    connect(this, &Theme::themeVariantChanged, this, &Theme::onThemeVariantChanged);
    connect(&themeManager, &ThemeManager::colorSchemeChanged, this, &Theme::onThemeVariantChanged);
    connect(&themeManager, &ThemeManager::accentColorChanged, this, &Theme::updateAccentColor);

    AppSettings settings;
    m_themeVariant = static_cast<ThemeVariant>(settings.value("generic/themeVariant", 0).toUInt());

    onThemeVariantChanged();

    // Setup listeners for dark mode
    connect(this, &Theme::isDarkModeChanged, this, &Theme::updateColorPalette);
    connect(this, &Theme::isDarkModeChanged, this, &Theme::updateAccentColor);

    updateColorPalette();
    updateAccentColor();
    useOwnDecoration();
}

bool Theme::useOwnDecoration()
{
    if (!m_useOwnDecorationInitalized) {
        m_useOwnDecorationInitalized = true;

        AppSettings settings;
        const auto settingsVal =
                settings.value("generic/useOwnWindowDecoration", "auto").toString();

        if (settingsVal == "auto") {
#ifdef Q_OS_LINUX
            const auto desktop =
                    QString::fromLocal8Bit(qgetenv("XDG_SESSION_DESKTOP")).toLower(); // gnome|kde
            m_useOwnDecoration = desktop.contains("gnome");
#else
            m_useOwnDecoration = true;
#endif
        } else {
            m_useOwnDecoration = settingsVal == "true";
        }
    }
    return m_useOwnDecoration;
}

QColor Theme::pickForegroundColor(const QColor &backgroundColor) const
{
    // WCAG 2.x relative luminance
    // https://www.w3.org/TR/WCAG21/#dfn-relative-luminance

    const auto luminance = relativeLuminance(backgroundColor);
    return (luminance > 0.5) ? seedInkLight : seedInkDark;
}

QColor Theme::readableOn(const QColor &background) const
{
    const QColor dark = m_primaryTextColor;
    const QColor light = m_foregroundWhiteColor;
    return contrastRatio(dark, background) >= contrastRatio(light, background) ? dark : light;
}

void Theme::setUseOwnDecoration(bool value)
{
    if (m_useOwnDecoration != value) {
        m_useOwnDecoration = value;
        AppSettings settings;
        settings.setValue("generic/useOwnWindowDecoration", value ? "true" : "false");
        Q_EMIT useOwnDecorationChanged();
    }
}

void Theme::onThemeVariantChanged()
{
    AppSettings settings;
    settings.setValue("generic/themeVariant", static_cast<uint>(m_themeVariant));

    switch (m_themeVariant) {

    case ThemeVariant::System: {
        const auto portalScheme = ThemeManager::instance().colorScheme();
        switch (portalScheme) {
        case ThemeManager::ColorScheme::NO_PREFERENCE:
            setDarkMode(false);
            break;

        case ThemeManager::ColorScheme::DARK:
            setDarkMode(true);
            break;

        case ThemeManager::ColorScheme::LIGHT:
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
    // Light mode/fallback colors
    m_primaryTextColor = seedInkLight;
    m_foregroundWhiteColor = seedPaperLight;
    m_foregroundHeaderIcons = textVeil(seedInkLight, 0.90);
    m_foregroundHeaderIconsInactive = textVeil(seedInkLight, 0.51);
    m_foregroundInitials = seedInitials;
    m_secondaryTextColor = textVeil(seedInkLight, 0.40);
    m_inactiveTextColor = textVeil(seedInkLight, 0.59);
    m_secondaryInactiveTextColor = textVeil(seedInkLight, 0.34);
    m_borderColor = neutralSurface(0.859);
    m_borderHeaderIconHovered = neutralSurface(0.808);
    m_backgroundColor = neutralSurface(1.000);
    m_backgroundSecondaryColor = neutralSurface(0.980);
    m_backgroundOffsetColor = textVeil(QColor(0, 0, 0), 20.0 / 255.0);
    m_backgroundOffsetHoveredColor = textVeil(QColor(0, 0, 0), 40.0 / 255.0);
    m_backgroundHeader = neutralSurface(0.922);
    m_backgroundHeaderInactive = neutralSurface(0.949);
    m_backgroundHeaderIconHovered = neutralSurface(0.973);
    m_backgroundInitials = mix(seedInitials, seedPaperLight, 0.82);
    m_paneColor = neutralSurface(0.965);
    m_highlightColor = textVeil(m_accentColor, 76.0 / 255.0);
    m_rttBubbleSelf = seedBubbleLight;
    m_rttTextSelf = readableOn(m_rttBubbleSelf);
    m_rttBubbleOther = neutralSurface(0.914);
    m_rttTextOther = readableOn(m_rttBubbleOther);

    // Extra colors (hard values)
    m_shadowColor = QColor(0, 0, 0, 32);
    m_redColor = QColor(224, 27, 36);
    m_orangeColor = QColor(245, 121, 0);
    m_yellowColor = QColor(217, 176, 114);
    m_emergencyColor = QColor(0, 136, 85);
    m_greenColor = QColor(36, 181, 27);
    m_darkGreenColor = QColor(128, 128, 0);
    m_activeIndicatorColor = QColor(255, 102, 0);

    // Dark mode
    if (m_isDarkMode) {
        m_primaryTextColor = seedInkDark;
        m_foregroundHeaderIcons = textVeil(seedVeilBaseDark, 0.97);
        m_foregroundHeaderIconsInactive = textVeil(seedVeilBaseDark, 0.65);
        m_secondaryTextColor = textVeil(seedVeilBaseDark, 0.60);
        m_inactiveTextColor = textVeil(seedVeilBaseDark, 0.35);
        m_secondaryInactiveTextColor = textVeil(seedVeilBaseDark, 0.24);
        m_borderColor = neutralSurface(0.129);
        m_borderHeaderIconHovered = neutralSurface(0.110);
        m_backgroundColor = neutralSurface(0.208);
        m_backgroundSecondaryColor = neutralSurface(0.275);
        m_backgroundOffsetColor = textVeil(seedVeilBaseDark, 20.0 / 255.0);
        m_backgroundOffsetHoveredColor = textVeil(seedVeilBaseDark, 40.0 / 255.0);
        m_backgroundHeader = neutralSurface(0.188);
        m_backgroundHeaderInactive = neutralSurface(0.141);
        m_backgroundHeaderIconHovered = neutralSurface(0.216);
        m_paneColor = neutralSurface(0.176);

        const QColor rawAccent = ThemeManager::instance().accentColor();
        m_highlightColor =
                textVeil(rawAccent.isValid() ? rawAccent : seedHighlightDark, 36.0 / 255.0);

        m_rttBubbleSelf = seedBubbleDark;
        m_rttTextSelf = readableOn(m_rttBubbleSelf);
        m_rttBubbleOther = neutralSurface(0.176);
        m_rttTextOther = readableOn(m_rttBubbleOther);
    }

    Q_EMIT colorPaletteChanged();
}

void Theme::updateAccentColor()
{
    auto newColor = ThemeManager::instance().accentColor();

    if (!newColor.isValid() || newColor == QColor(Qt::transparent)) {
        newColor = m_isDarkMode ? seedAccentDark : seedAccentLight;
    }

    if (m_accentColor != newColor) {
        m_accentColor = newColor;
        Q_EMIT accentColorChanged();
    }
}

void Theme::setDarkMode(bool value)
{
    if (m_isDarkMode != value) {
        m_isDarkMode = value;
        Q_EMIT isDarkModeChanged();
    }
}
