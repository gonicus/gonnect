#include "TrackDecoder.h"
#include "FramePump.h"

#include <QLoggingCategory>
#include <QVideoFrame>

Q_LOGGING_CATEGORY(lcTrackDecoder, "gonnect.media.video.decoder")

TrackDecoder::TrackDecoder(const QString &trackId, QObject *parent)
    : QObject{ parent }, m_trackId{ trackId }
{
}

void TrackDecoder::enqueue(EncodedFrame frame, std::shared_ptr<FramePump> output)
{
    ++m_queuedFrames;

    QMetaObject::invokeMethod(
            this,
            [this, frame = std::move(frame), output = std::move(output)] {
                --m_queuedFrames;
                decode(frame, output);
            },
            Qt::QueuedConnection);
}

void TrackDecoder::decode(const EncodedFrame &frame, const std::shared_ptr<FramePump> &output)
{
    if (m_decoder.codec() != frame.codec) {
        const auto opened = m_decoder.open(frame.codec);
        if (!opened) {
            qCWarning(lcTrackDecoder) << "Cannot open decoder for" << m_trackId << opened.error();
            Q_EMIT decodeFailed();
            return;
        }

        qCInfo(lcTrackDecoder) << "Decoding" << m_trackId << "as"
                               << VideoDecoder::codecName(frame.codec);
    }

    const auto decoded = m_decoder.decode(frame.payload);
    if (!decoded) {
        qCWarning(lcTrackDecoder) << "Cannot decode frame of" << m_trackId << decoded.error();
        Q_EMIT decodeFailed();
        return;
    }

    for (const QVideoFrame &videoFrame : *decoded) {
        if (videoFrame.size() != m_size) {
            m_size = videoFrame.size();
            qCInfo(lcTrackDecoder) << "Size of" << m_trackId << "is now" << m_size;
        }

        if (output) {
            output->push(videoFrame);
        }
    }
}
