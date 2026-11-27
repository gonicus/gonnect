#include "CameraSource.h"

CameraSource::CameraSource(QObject *parent) : QObject{ parent } { }

void CameraSource::setFrameHandler(FrameHandler handler)
{
    QMutexLocker locker(&m_frameHandlerMutex);
    m_frameHandler = std::move(handler);
}

void CameraSource::deliverFrame(const QVideoFrame &frame)
{
    QMutexLocker locker(&m_frameHandlerMutex);

    if (m_frameHandler) {
        m_frameHandler(frame);
    }
}
