#pragma once

#include "FramePump.h"

#include <QObject>
#include <qqmlregistration.h>

#include <memory>

/// Video of conference participant, abstracted from specific backend.
/// Decoded frames are pushed into frame pump from any thread.
class ConferenceVideoSource : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("RegisterEnumClassesUnscoped", "false")
    QML_ELEMENT
    QML_UNCREATABLE("")

    Q_PROPERTY(QString id READ id CONSTANT FINAL)
    Q_PROPERTY(QString userId READ userId CONSTANT FINAL)
    Q_PROPERTY(ConferenceVideoSource::Type type READ type CONSTANT FINAL)
    Q_PROPERTY(bool isMuted READ isMuted NOTIFY isMutedChanged FINAL)
    Q_PROPERTY(int requestedHeight READ requestedHeight NOTIFY requestedHeightChanged FINAL)

public:
    enum class Type { Camera, ScreenShare };
    Q_ENUM(Type)

    ConferenceVideoSource(const QString &id, const QString &userId, Type type,
                          QObject *parent = nullptr);
    ~ConferenceVideoSource() override;

    QString id() const { return m_id; }
    QString userId() const { return m_userId; }
    Type type() const { return m_type; }

    bool isMuted() const { return m_isMuted; }
    void setIsMuted(bool value);

    /// Height of displayed video; 0 if not visible
    int requestedHeight() const { return m_requestedHeight; }
    void setRequestedHeight(int height);

    /// Shared with decoder threads -> pump may outlive this source
    std::shared_ptr<FramePump> framePump() const { return m_framePump; }

Q_SIGNALS:
    void isMutedChanged();
    void requestedHeightChanged();

private:
    QString m_id;
    QString m_userId;
    Type m_type = Type::Camera;
    bool m_isMuted = false;
    int m_requestedHeight = 0;
    std::shared_ptr<FramePump> m_framePump;
};
