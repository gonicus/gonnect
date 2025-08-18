#pragma once

#include <QObject>
#include <QQmlEngine>

class EmergencyContact;

class GlobalInfo : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool hasEmergencyNumbers READ hasEmergencyNumbers CONSTANT FINAL)

public:
    static GlobalInfo &instance()
    {
        static GlobalInfo *_instance = nullptr;
        if (!_instance) {
            _instance = new GlobalInfo;
        }
        return *_instance;
    }

    Q_INVOKABLE QString jitsiUrl();

    bool hasEmergencyNumbers();
    const QList<EmergencyContact *> &emergencyContacts();

private:
    explicit GlobalInfo(QObject *parent = nullptr);
    void initEmergencyContacts();

    bool m_isJitsiUrlInitialized = false;
    bool m_hasEmergencyNumbersInitialized = false;
    QString m_jitsiUrl;
    QList<EmergencyContact *> m_emergencyContacts;
};

class GlobalInfoWrapper
{
    Q_GADGET
    QML_FOREIGN(GlobalInfo)
    QML_NAMED_ELEMENT(GlobalInfo)
    QML_SINGLETON

public:
    static GlobalInfo *create(QQmlEngine *, QJSEngine *) { return &GlobalInfo::instance(); }

private:
    GlobalInfoWrapper() = default;
};
