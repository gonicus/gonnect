#pragma once

#include <QObject>

class JitsiMediaDevice : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("RegisterEnumClassesUnscoped", "false")

public:
    enum class Type { Unknown, AudioInput, AudioOutput, VideoInput };
    Q_ENUM(Type)

    explicit JitsiMediaDevice(const QString &deviceId, const QString &groupId, const Type type,
                              const QString &label = "", QObject *parent = nullptr);

    QString deviceId() const { return m_deviceId; }
    QString groupId() const { return m_groupId; }
    QString label() const { return m_label; }
    Type type() const { return m_type; }

private:
    QString m_deviceId;
    QString m_groupId;
    QString m_label;
    Type m_type = Type::Unknown;
};
