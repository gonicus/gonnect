#pragma once
#include <QObject>
#include "SIPTemplateField.h"

class QSettings;

class SIPTemplate : public QObject
{
    Q_OBJECT

public:
    explicit SIPTemplate(const QString &path, QObject *parent = nullptr);

    bool isValid() const { return m_valid; }

    QString id() const { return m_id; }
    QString name() const { return m_name; }
    QList<SIPTemplateField *> fields() const { return m_fields; }

    QString save(const QVariantMap &values) const;

private:
    QString i18nValue(const QString &item);

    QString m_id;
    QString m_name;
    QList<SIPTemplateField *> m_fields;

    QSettings *m_settings = nullptr;

    bool m_plain = false;
    bool m_valid = false;
};
