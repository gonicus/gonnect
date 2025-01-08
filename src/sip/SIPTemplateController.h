#pragma once

#include <QObject>
#include <QQmlEngine>

class SIPTemplateController : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit SIPTemplateController(QObject *parent = nullptr);

    Q_INVOKABLE QVariantMap createConfig(const QString &templateId, const QVariantMap &values) const;
};
