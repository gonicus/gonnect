#include "FramePump.h"

#include <QVideoSink>

FramePump::FramePump(QObject *parent) : QObject{ parent } { }

void FramePump::setSink(QVideoSink *sink)
{
    m_sink = sink;
}

void FramePump::push(QVideoFrame frame)
{
    {
        QMutexLocker locker(&m_mutex);

        if (m_pending.isValid()) {
            ++m_dropped;
        }

        m_pending = std::move(frame);

        if (m_deliveryScheduled) {
            return;
        }
        m_deliveryScheduled = true;
    }

    QMetaObject::invokeMethod(this, &FramePump::deliver, Qt::QueuedConnection);
}

quint64 FramePump::droppedFrames() const
{
    QMutexLocker locker(&m_mutex);
    return m_dropped;
}

void FramePump::deliver()
{
    QVideoFrame frame;
    {
        QMutexLocker locker(&m_mutex);
        frame = std::exchange(m_pending, QVideoFrame());
        m_deliveryScheduled = false;
    }

    if (m_sink) {
        m_sink->setVideoFrame(frame);
    }
}
