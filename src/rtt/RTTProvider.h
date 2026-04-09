#pragma once

#include <QObject>
#include <QtQml/qqml.h>
#include <QtQml/qqmlregistration.h>

#include "RTTModel.h"
#include "SIPCallManager.h"

class RTTProvider : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(RTTProvider)

    Q_PROPERTY(RTTModel *model READ model CONSTANT)

public:
    Q_REQUIRED_RESULT static RTTProvider &instance()
    {
        static RTTProvider *_instance = nullptr;

        if (_instance == nullptr) {
            _instance = new RTTProvider();
        }

        return *_instance;
    }

    ~RTTProvider() = default;

    RTTModel *model() const { return m_model; }

Q_SIGNALS:

private:
    RTTProvider(QObject *parent = nullptr);

    RTTModel *m_model = nullptr;
    SIPCallManager *m_callManager = nullptr;
};

class RTTProviderWrapper
{
    Q_GADGET
    QML_FOREIGN(RTTProvider)
    QML_NAMED_ELEMENT(RTTProvider)
    QML_SINGLETON

public:
    static RTTProvider *create(QQmlEngine *, QJSEngine *) { return &RTTProvider::instance(); }

private:
    RTTProviderWrapper() = default;
};
