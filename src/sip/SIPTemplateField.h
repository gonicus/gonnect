#pragma once
#include <QObject>
#include <QRegularExpression>
#include <QtQml/qqml.h>
#include <QtQml/qqmlregistration.h>

class SIPTemplateField : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

public:
    explicit SIPTemplateField(QObject *parent = nullptr) : QObject{ parent } { }

    enum class TemplateFieldType { Invalid, Text, Secret, File };
    Q_ENUM(TemplateFieldType)

    QString name;
    QString description;
    QString preset;
    QString target;
    QString mimeType;
    TemplateFieldType type = SIPTemplateField::TemplateFieldType::Invalid;
    QRegularExpression regex;
    bool required = true;
};
