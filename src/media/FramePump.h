#pragma once

#include <QObject>
#include <QMutex>
#include <QPointer>
#include <QVideoFrame>

class QVideoSink;

// Make sure only the latest frame is kept.
class FramePump : public QObject
{
    Q_OBJECT

public:
    explicit FramePump(QObject *parent = nullptr);

    void setSink(QVideoSink *sink);

    /// Callable from any thread
    void push(QVideoFrame frame);

    quint64 droppedFrames() const;

private:
    void deliver();

    QPointer<QVideoSink> m_sink;
    mutable QMutex m_mutex;
    QVideoFrame m_pending;
    bool m_deliveryScheduled = false;
    quint64 m_dropped = 0;
};
