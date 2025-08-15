#pragma once

#include <QObject>
#include <QQmlEngine>

class GlobalInfo : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("RegisterEnumClassesUnscoped", "false")

public:
    enum class WorkaroundId { GOW_001 };
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

private:
    explicit GlobalInfo(QObject *parent = nullptr);

    bool m_isJitsiUrlInitialized = false;
    QHash<WorkaroundId, bool> m_workaroundActiveCache;
    QString m_jitsiUrl;
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
