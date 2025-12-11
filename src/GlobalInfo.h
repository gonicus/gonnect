#pragma once

#include <QObject>
#include <QQmlEngine>

class EmergencyContact;

class GlobalInfo : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("RegisterEnumClassesUnscoped", "false")
    Q_PROPERTY(bool hasEmergencyNumbers READ hasEmergencyNumbers CONSTANT FINAL)

public:
    enum class WorkaroundId { GOW_001, GOW_002, GOW_003 };
    Q_ENUM(WorkaroundId)

    static QString workaroundIdToString(const WorkaroundId enumValue)
    {
        return QMetaEnum::fromType<WorkaroundId>().valueToKey(static_cast<int>(enumValue));
    }

    static GlobalInfo &instance()
    {
        static GlobalInfo *_instance = nullptr;
        if (!_instance) {
            _instance = new GlobalInfo;
        }
        return *_instance;
    }

    Q_INVOKABLE QString jitsiUrl();

    Q_INVOKABLE bool isWorkaroundActive(const WorkaroundId id);

    bool hasEmergencyNumbers();
    const QList<EmergencyContact *> &emergencyContacts();

private:
    explicit GlobalInfo(QObject *parent = nullptr);
    void initEmergencyContacts();

    bool m_isJitsiUrlInitialized = false;

    QHash<WorkaroundId, bool> m_workaroundActiveCache;

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
