#include "ConferenceVideoOutput.h"

ConferenceVideoOutput::ConferenceVideoOutput(QObject *parent) : QObject{ parent } { }

ConferenceVideoOutput::~ConferenceVideoOutput()
{
    detach();
}

void ConferenceVideoOutput::setSource(ConferenceVideoSource *source)
{
    if (m_source != source) {
        detach();
        m_source = source;
        attach();
        Q_EMIT sourceChanged();
    }
}

void ConferenceVideoOutput::setVideoSink(QVideoSink *sink)
{
    if (m_videoSink != sink) {
        m_videoSink = sink;
        if (m_source) {
            m_source->framePump()->setSink(sink);
        }
        Q_EMIT videoSinkChanged();
    }
}

void ConferenceVideoOutput::setDisplayedHeight(int height)
{
    if (m_displayedHeight != height) {
        m_displayedHeight = height;
        if (m_source) {
            m_source->setRequestedHeight(height);
        }
        Q_EMIT displayedHeightChanged();
    }
}

void ConferenceVideoOutput::attach()
{
    if (m_source) {
        m_source->framePump()->setSink(m_videoSink);
        m_source->setRequestedHeight(m_displayedHeight);
    }
}

void ConferenceVideoOutput::detach()
{
    if (m_source) {
        m_source->framePump()->setSink(nullptr);
        m_source->setRequestedHeight(0);
    }
}
