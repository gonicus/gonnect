#pragma once
#include <QObject>
#include <QtQml/qqml.h>
#include <QtQml/qqmlregistration.h>
#include "SIPTemplate.h"

class QSettings;

class SIPTemplates : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(SIPTemplates)

public:
    Q_REQUIRED_RESULT static SIPTemplates &instance()
    {
        static SIPTemplates *_instance = nullptr;

        if (_instance == nullptr) {
            _instance = new SIPTemplates();
        }

        return *_instance;
    }

    static QStringList suffixesForMimeType(const QString &mimeType);

    QList<SIPTemplate *> templates() const { return m_templates; }
    SIPTemplate *templateById(const QString &id) const;

    ~SIPTemplates() = default;

private:
    SIPTemplates(QObject *parent = nullptr);

    QList<SIPTemplate *> m_templates;
};

class SIPTemplatesWrapper
{
    Q_GADGET
    QML_FOREIGN(SIPTemplates)
    QML_NAMED_ELEMENT(SIPTemplates)
    QML_SINGLETON

public:
    static SIPTemplates *create(QQmlEngine *, QJSEngine *) { return &SIPTemplates::instance(); }

private:
    SIPTemplatesWrapper() = default;
};
