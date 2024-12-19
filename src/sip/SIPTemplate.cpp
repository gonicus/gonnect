#include <QLoggingCategory>
#include <QDirIterator>
#include <QSettings>
#include <QStandardPaths>
#include <QCoreApplication>

#include "SIPTemplate.h"

SIPTemplate::SIPTemplate(const QString &path, QObject *parent) : QObject(parent), m_id(path)
{
    m_settings = new QSettings(path, QSettings::Format::IniFormat);

    m_name = i18nValue("template/name");
    if (m_name.isEmpty()) {
        return;
    }

    m_plain = m_settings->value("template/plain", false).toBool();
    m_valid = true;

    const auto keys = m_settings->childGroups();
    for (const QString &key : keys) {
        if (key.startsWith("templateField")) {
            auto field = new SIPTemplateField(this);
            m_fields.append(field);

            field->name = i18nValue(key + "/name");
            field->description = i18nValue(key + "/description");
            field->target = m_settings->value(key + "/target").toString();
            field->mimeType = m_settings->value(key + "/mimeType").toString();
            field->required = m_settings->value(key + "/required", true).toBool();

            auto reg = m_settings->value(key + "/regex").toString();
            if (reg.isEmpty()) {
                field->regex = QRegularExpression(".*");
            } else {
                field->regex = QRegularExpression(reg);
            }

            auto type = m_settings->value(key + "/type").toString();
            if (type == "text") {
                field->type = SIPTemplateField::TemplateFieldType::Text;
            } else if (type == "secret") {
                field->type = SIPTemplateField::TemplateFieldType::Secret;
            } else if (type == "file") {
                field->type = SIPTemplateField::TemplateFieldType::File;
            }
        }
    }
}

QString SIPTemplate::save(const QVariantMap &values) const
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/"
            + QCoreApplication::organizationName();

    QDir dir(path);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    path += "/99-user.conf";
    if (QFile::exists(path)) {
        if (!QFile::remove(path)) {
            return tr("Error: failed to write to %1").arg(path);
        }
    }

    if (m_plain) {
        if (!QFile::copy(m_id, path)) {
            return tr("Error: failed to write to %1").arg(path);
        }
    } else {
        const QSettings source(m_id, QSettings::Format::IniFormat);
        QSettings destination(path, QSettings::Format::IniFormat);

        const QStringList keys = source.allKeys();
        for (const auto &key : keys) {
            QString targetValue = source.value(key).toString();

            if (key.startsWith("template")) {
                continue;
            }

            for (auto it = values.constBegin(); it != values.constEnd(); ++it) {
                QString sourcePattern = "%TPL[" + it.key() + "]%";
                targetValue.replace(sourcePattern, it.value().toString());
            }

            destination.setValue(key, targetValue);
        }

        destination.sync();
    }

    // The path is only used to display the config file location. In case
    // of living in a flatpak, the host path is different and will not help
    // the user to find it. Adjust it here.
    if (qEnvironmentVariable("container") == "flatpak") {
        path = qEnvironmentVariable("XDG_CONFIG_HOME") + "/" + QCoreApplication::organizationName()
                + "/99-user.conf";
    }

    return path;
}

QString SIPTemplate::i18nValue(const QString &item)
{
    QString ln = QLocale::system().name();

    QString value = m_settings->value(item + "[" + ln + "]").toString();
    if (value.isEmpty()) {
        ln = ln.split("_").first();
        value = m_settings->value(item + "[" + ln + "]").toString();

        if (value.isEmpty()) {
            value = m_settings->value(item).toString();
        }
    }

    return value;
}