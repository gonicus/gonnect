#include "ConferenceVideoSource.h"

ConferenceVideoSource::ConferenceVideoSource(const QString &id, const QString &userId, Type type,
                                             QObject *parent)
    : QObject{ parent },
      m_id{ id },
      m_userId{ userId },
      m_type{ type },
      m_framePump{ new FramePump, [](FramePump *pump) { pump->deleteLater(); } }
{
}

ConferenceVideoSource::~ConferenceVideoSource()
{
    m_framePump->setSink(nullptr);
}

void ConferenceVideoSource::setIsMuted(bool value)
{
    if (m_isMuted != value) {
        m_isMuted = value;
        Q_EMIT isMutedChanged();
    }
}

void ConferenceVideoSource::setRequestedHeight(int height)
{
    if (m_requestedHeight != height) {
        m_requestedHeight = height;
        Q_EMIT requestedHeightChanged();
    }
}
