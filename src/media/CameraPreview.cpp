#include "CameraPreview.h"
#include "VideoManager.h"

CameraPreview::CameraPreview(QObject *parent) : QObject{ parent } { }

CameraPreview::~CameraPreview()
{
    if (m_isActive) {
        VideoManager::instance().removeConsumer(&m_framePump);
    }
}

void CameraPreview::setVideoSink(QVideoSink *sink)
{
    if (m_videoSink != sink) {
        m_videoSink = sink;
        m_framePump.setSink(sink);
        Q_EMIT videoSinkChanged();
    }
}

void CameraPreview::setIsActive(bool value)
{
    if (m_isActive != value) {
        m_isActive = value;

        if (m_isActive) {
            VideoManager::instance().addConsumer(&m_framePump);
        } else {
            VideoManager::instance().removeConsumer(&m_framePump);
            m_framePump.clear();
        }

        Q_EMIT isActiveChanged();
    }
}
