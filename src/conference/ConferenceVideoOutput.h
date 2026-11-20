#pragma once

#include "ConferenceVideoSource.h"

#include <QObject>
#include <QPointer>
#include <QVideoSink>
#include <qqmlregistration.h>

/// Feeds conference video source into VideoOutput.
/// Each source feeds only one sink: last attached output wins.
class ConferenceVideoOutput : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(ConferenceVideoSource *source READ source WRITE setSource NOTIFY sourceChanged FINAL)
    Q_PROPERTY(
            QVideoSink *videoSink READ videoSink WRITE setVideoSink NOTIFY videoSinkChanged FINAL)
    Q_PROPERTY(int displayedHeight READ displayedHeight WRITE setDisplayedHeight NOTIFY
                       displayedHeightChanged FINAL)

public:
    explicit ConferenceVideoOutput(QObject *parent = nullptr);
    ~ConferenceVideoOutput() override;

    ConferenceVideoSource *source() const { return m_source; }
    void setSource(ConferenceVideoSource *source);

    QVideoSink *videoSink() const { return m_videoSink; }
    void setVideoSink(QVideoSink *sink);

    /// Device pixels; 0 if invisible
    int displayedHeight() const { return m_displayedHeight; }
    void setDisplayedHeight(int height);

Q_SIGNALS:
    void sourceChanged();
    void videoSinkChanged();
    void displayedHeightChanged();

private:
    void attach();
    void detach();

    QPointer<ConferenceVideoSource> m_source;
    QPointer<QVideoSink> m_videoSink;
    int m_displayedHeight = 0;
};
