#pragma once

#include <QObject>
#include <QtQml/qqml.h>
#include <QtQml/qqmlregistration.h>
#include <QSettings>
#include <QStandardPaths>
#include <QLibraryInfo>
#include <QLoggingCategory>
#include <QUuid>

class UISettings : public QSettings
{
    Q_OBJECT

public:
    Q_REQUIRED_RESULT static UISettings &instance()
    {
        static UISettings *_instance = nullptr;

        if (_instance == nullptr) {
            _instance = new UISettings();
        }

        return *_instance;
    }

    ~UISettings() = default;

    Q_INVOKABLE QVariant getUISetting(const QString &group, const QString &key,
                                      const QVariant &defaultValue);
    Q_INVOKABLE void setUISetting(const QString &group, const QString &key, const QVariant &value);
    Q_INVOKABLE void removeUISetting(const QString &group, const QString &key);

    Q_INVOKABLE QStringList getPageIds();
    Q_INVOKABLE QStringList getWidgetIds();

    Q_INVOKABLE QString generateUuid();

private:
    explicit UISettings(QObject *parent = nullptr);
};

class UISettingsWrapper
{
    Q_GADGET
    QML_FOREIGN(UISettings)
    QML_NAMED_ELEMENT(UISettings)
    QML_SINGLETON

public:
    static UISettings *create(QQmlEngine *, QJSEngine *) { return &UISettings::instance(); }

private:
    UISettingsWrapper() = default;
};
