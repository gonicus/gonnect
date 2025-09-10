#include <QLoggingCategory>
#include <QAudioSink>
#include <QMediaDevices>
#include <SIPAudioDevice.h>
#include <pjmedia/port.h>
#include "AudioPort.h"
Q_LOGGING_CATEGORY(lcAudioPort, "gonnect.sip.audio")

using namespace std::chrono_literals;

AudioPort::AudioPort(QAudioDevice device) : m_device(device)
{
    m_idleTimer.setInterval(1s);
    connect(&m_idleTimer, &QTimer::timeout, this, &AudioPort::stopIO);

    connect(this, &AudioPort::startIdleTimer, this,
            [this]() { QTimer::singleShot(0, this, [this]() { m_idleTimer.start(); }); });
}

AudioPort::~AudioPort()
{
    stopIO();
}

bool AudioPort::initialize()
{
    if (!initFmt()) {
        // Could not find a supported format for the audio device
        return false;
    }

    createPort(m_device.id().toStdString(), m_pj_fmt);

    return true;
}

void AudioPort::setMuted(bool value)
{
    m_isMuted = value;
}

bool AudioPort::initFmt()
{
    // By default assume 16 bit signed integer linear PCM with 16kHz sampling
    // rate and 20ms frame time. These settings are identical to the
    // implementation of the G.722 codec in pjsip, see
    // https://docs.pjsip.org/en/latest/api/generated/pjmedia/group/group__PJMED__G722.html
    pj_uint32_t formatID = PJMEDIA_FORMAT_L16;
    unsigned int clockRate = 16000;
    unsigned int channelCount = 1;
    int frameTimeUsec = 20000;
    int bitsPerSample = 16;

    QAudioFormat format;
    format.setSampleRate(clockRate);
    format.setChannelCount(channelCount);
    format.setSampleFormat(QAudioFormat::Int16);

    // Check if the the format is suitable for the device. If it is not set the
    // format to the preferred format of the device.
    if (!m_device.isFormatSupported(format)) {
        format = m_device.preferredFormat();
        format.setSampleFormat(QAudioFormat::Int16);
    }

    if (!m_device.isFormatSupported(format)) {
        // TODO: Better support for devices that do not support 16 bit PCM.
        return false;
    }

    m_pj_fmt.init(formatID, clockRate, channelCount, frameTimeUsec, bitsPerSample);
    m_pj_fmt.type = PJMEDIA_TYPE_AUDIO;
    m_audioFormat = format;

    return true;
}

void AudioPort::updateAudioLevel(const char *data, qint64 size)
{
    qreal max = 0;
    const qint64 numSamples = size / sizeof(qint16);

    static constexpr qreal positiveRange = std::numeric_limits<qint16>().max();

    for (int i = 0; i < numSamples; ++i) { // i is index of sample
        const auto word = static_cast<qint16>(*data);
        const qreal realValue = static_cast<qreal>(word) / positiveRange * 100.0;

        max = std::max(max, realValue);
        data += sizeof(qint16);
    }

    max = QtAudio::convertVolume(max, QtAudio::LinearVolumeScale, QtAudio::LogarithmicVolumeScale);
    setSourceAudioLevel(max);
}

void AudioPort::setSourceAudioLevel(qreal level)
{
    if (m_sourceAudioLevel != level) {
        m_sourceAudioLevel = level;
        Q_EMIT sourceLevelChanged(level);
    }
}

void AudioPort::setAudioDevice(QAudioDevice device)
{
    m_device = device;
    stopIO();
}

void AudioPort::startIO()
{
    if (m_device.mode() == QAudioDevice::Mode::Input) {
        startSourceIO();
    } else {
        startSinkIO();
    }
}

void AudioPort::stopIO()
{
    if (m_device.mode() == QAudioDevice::Mode::Input) {
        stopSourceIO();
    } else {
        stopSinkIO();
    }
}

void AudioPort::startSinkIO()
{
    m_idleTimer.stop();

    if (!m_sink.isNull()) {
        stopSinkIO();
        delete m_sink;
        m_sink = nullptr;
    }

    qCInfo(lcAudioPort).noquote().nospace()
            << "Initialize sink of device_descr=\"" << m_device.description() << "\", device_id=\""
            << m_device.id() << "\", with settings:"
            << "\nsampleRate=" << m_audioFormat.sampleRate()
            << "\nchannelCount=" << m_audioFormat.channelCount()
            << "\nbytesPerSample=" << m_audioFormat.bytesPerSample()
            << "\nsampleFormat=" << m_audioFormat.sampleFormat();

    m_sink = new QAudioSink(m_device, m_audioFormat);
    m_io = m_sink->start();

    Q_EMIT audioSinkChanged();
}

void AudioPort::stopSinkIO()
{
    m_idleTimer.stop();

    if (m_sink) {
        m_sink->stop();
        delete m_sink;
        m_sink = nullptr;
    }
    if (m_io) {
        delete m_io;
        m_io = nullptr;
    }

    Q_EMIT audioSinkChanged();
}

void AudioPort::startSourceIO()
{
    m_idleTimer.stop();

    if (!m_source.isNull()) {
        stopSourceIO();
        delete m_source;
        m_source = nullptr;
    }

    qCInfo(lcAudioPort).noquote().nospace()
            << "Initialize source of device_descr=\"" << m_device.description()
            << "\", device_id=\"" << m_device.id() << "\", with settings:"
            << "\nsampleRate=" << m_audioFormat.sampleRate()
            << "\nchannelCount=" << m_audioFormat.channelCount()
            << "\nbytesPerSample=" << m_audioFormat.bytesPerSample()
            << "\nsampleFormat=" << m_audioFormat.sampleFormat();

    m_source = new QAudioSource(m_device, m_audioFormat);
    m_io = m_source->start();

    Q_EMIT audioSourceChanged();
}

void AudioPort::stopSourceIO()
{
    m_idleTimer.stop();
    if (m_source) {
        m_source->stop();
        delete m_source;
        m_source = nullptr;
    }

    if (m_io) {
        delete m_io;
        m_io = nullptr;
    }

    Q_EMIT audioSourceChanged();
}

QString AudioPort::getDeviceID() const
{
    return SIPAudioDevice::makeHash(m_device.description(), false);
}

QString AudioPort::getSystemDeviceID() const
{
    return m_device.id();
}

void AudioPort::onFrameRequested(pj::MediaFrame &frame)
{
    if (m_device.mode() != QAudioDevice::Mode::Input) {
        return;
    }

    // Create source on the fly if needed
    if (m_source.isNull()) {
        startIO();
    }

    if (m_io.isNull()) {
        qCWarning(lcAudioPort) << "frame requested, but no IO available";
        return;
    }

    auto bytes = m_io->read(frame.size);

    if (!m_isMuted) {
        frame.buf = std::vector<unsigned char>(bytes.constBegin(), bytes.constEnd());
        frame.type = PJMEDIA_FRAME_TYPE_AUDIO;
        updateAudioLevel(bytes, bytes.size());
    } else {
        setSourceAudioLevel(0);
    }

    // Auto destroy sink after timeout
    Q_EMIT startIdleTimer();
}

void AudioPort::onFrameReceived(pj::MediaFrame &frame)
{
    if (m_device.mode() == QAudioDevice::Mode::Input) {
        return;
    }

    // Create sink on the fly if needed
    if (m_sink.isNull()) {
        startIO();
    }

    if (m_io.isNull()) {
        qCWarning(lcAudioPort) << "frame received, but no IO available";
        return;
    }

    m_io->write(reinterpret_cast<char *>(frame.buf.data()), frame.size);

    // Auto destroy sink after timeout
    Q_EMIT startIdleTimer();
}
