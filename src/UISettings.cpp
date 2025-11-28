#include "UISettings.h"

#include <QRegularExpression>

Q_LOGGING_CATEGORY(lcUISettings, "gonnect.ui.settings")

// TODO: Custom config for UI? E.g. '98-ui.conf'?
UISettings::UISettings(QObject *parent)
    : QSettings(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
                        + "/gonnect/99-user.conf",
                QSettings::IniFormat, parent)
{
}

QVariant UISettings::getUISetting(const QString &group, const QString &key,
                                  const QVariant &defaultValue)
{
    QString compoundSetting = QString("%1/%2").arg(group, key);
    return value(compoundSetting, defaultValue);
}

void UISettings::setUISetting(const QString &group, const QString &key, const QVariant &value)
{
    QString compoundSetting = QString("%1/%2").arg(group, key);
    setValue(compoundSetting, value);
}

void UISettings::removeUISetting(const QString &group, const QString &key)
{
    beginGroup(group);
    remove(key); // INFO: If key == "", the entire group is deleted!
    endGroup();
}

QStringList UISettings::getPageIds()
{
    static const QRegularExpression dynamicPageName("^page_[0-9a-f]{32}$",
                                                    QRegularExpression::CaseInsensitiveOption);

    const QStringList keys = childGroups();
    QStringList matchingKeys;

    for (const QString &key : keys) {
        // "page_XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
        if (dynamicPageName.match(key).hasMatch()) {
            matchingKeys.append(key);
        }
    }
    return matchingKeys;
}

QStringList UISettings::getWidgetIds(QString pageId)
{

    const QStringList keys = childGroups();
    QStringList matchingKeys;

    for (const QString &key : keys) {
        // "page_XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX-widget_YYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY"
        if (key.startsWith(QString("%1-widget_").arg(pageId))) {
            matchingKeys.append(key);
        }
    }
    return matchingKeys;
}

QString UISettings::generateUuid()
{
    // Variant QUuid::DCE and version QUuid::UnixEpoch
    // as 128 bit in hex without dashes or braces
    return QUuid::createUuidV7().toString(QUuid::Id128);
}
