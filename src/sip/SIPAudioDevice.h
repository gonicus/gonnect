#pragma once
#include <QObject>
#include <QUuid>
#include <QtQml/qqml.h>
#include <QtQml/qqmlregistration.h>
#include <QAudioDevice>
#include <pjsua2.hpp>

class SIPAudioDevice : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name CONSTANT FINAL)
    Q_PROPERTY(QString id READ uniqueId CONSTANT FINAL)
    Q_PROPERTY(bool isInput READ isInput CONSTANT FINAL)
    Q_PROPERTY(bool isOutput READ isOutput CONSTANT FINAL)

    QML_ELEMENT
    QML_UNCREATABLE("This object is managed in C++")
    Q_DISABLE_COPY(SIPAudioDevice)

public:
    SIPAudioDevice(const QString &name, bool input, bool defaultDevice, QObject *parent = nullptr);
    ~SIPAudioDevice();

    QString name() const { return m_name; }
    QString uniqueId() const { return m_hash; }

    bool isInput() const { return m_isInput; }
    bool isOutput() const { return !m_isInput; }
    bool isDefault() const { return m_default; }

    static QString makeHash(const QString &name, bool input);

private:
    bool m_isInput = false;
    bool m_default = false;

    QString m_hash;
    QString m_name;
};
