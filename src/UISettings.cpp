#include "UISettings.h"

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

// TODO: Use "page_XXXXXX-widget_YYYYYY" format?
QStringList UISettings::getPageIds()
{
    QStringList keys = allKeys();
    QStringList matchingKeys;

    // "pageXXXXXX"
    for (const QString &key : keys) {
        if (key.startsWith("page") && !key.contains("_widget")) {
            matchingKeys.append(key);
        }
    }
    return matchingKeys;
}

QStringList UISettings::getWidgetIds(QString pageId)
{
    QStringList keys = allKeys();
    QStringList matchingKeys;

    // "pageXXXXXX_widgetYYYYYY"
    for (const QString &key : keys) {
        if (key.startsWith(pageId+"_widget")) {
            matchingKeys.append(key);
        }
    }
    return matchingKeys;
}

QString UISettings::generateUuid()
{
    // Variant QUuid::DCE and version QUuid::UnixEpoch
    return QUuid::createUuidV7().toString(QUuid::WithoutBraces);
}
