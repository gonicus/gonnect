#pragma once
#include <QObject>
#include <QtQml/qqml.h>
#include <QtQml/qqmlregistration.h>
#include <pjsua2.hpp>

class SIPManager;

class SIPUserAgentConfig : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("This object is managed by SIPManager and cannot be created")
    Q_DISABLE_COPY(SIPUserAgentConfig)

public:
    SIPUserAgentConfig(SIPManager *parent = nullptr);
    ~SIPUserAgentConfig() = default;

    void applyConfig(pj::EpConfig &epConfig);
};
