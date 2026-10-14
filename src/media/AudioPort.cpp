#include <QLoggingCategory>
#include <QAudioSink>
#include <QMediaDevices>
#include <SIPAudioDevice.h>
#include <pjmedia/port.h>
#include <mutex>
#include "AudioPort.h"
#include "AudioProcessor.h"

#ifdef Q_OS_LINUX
#  include <QDBusConnection>
#  include <QDBusConnectionInterface>
#  include <QDBusInterface>
#  include <QDBusReply>
#  include <sys/resource.h>
#  include <sys/syscall.h>
#endif

Q_LOGGING_CATEGORY(lcAudioPort, "gonnect.sip.audio")

#define NORMAL_AUDIO_LEVEL 1.6f
#define SILENCE_BUFFER_MS 1000
#define SILENCE_TARGET_FILL_MS 80
#define MEDIA_THREAD_RT_PRIORITY 20
#define MEDIA_THREAD_RTTIME_USEC 200000

using namespace std::chrono_literals;

namespace {

#ifdef Q_OS_LINUX

struct RealtimeEndpoint
{
    QDBusConnection bus;
    QString service;
    QString path;
    QString interface;
};

std::optional<RealtimeEndpoint> findRealTimeEndpoint()
{
    // Try portal first...
    QDBusConnection sessionBus = QDBusConnection::sessionBus();
    if (sessionBus.isConnected()
        && sessionBus.interface()
                   ->isServiceRegistered(QStringLiteral("org.freedesktop.portal.Desktop"))
                   .value()) {
        qCDebug(lcAudioPort()) << "realtime endpoint: picking org.freedesktop.portal.Desktop";
        return RealtimeEndpoint{ sessionBus, QStringLiteral("org.freedesktop.portal.Desktop"),
                                 QStringLiteral("/org/freedesktop/portal/desktop"),
                                 QStringLiteral("org.freedesktop.portal.Realtime") };
    }

    // ... then the system bus as a fallback
    QDBusConnection systemBus = QDBusConnection::systemBus();
    if (systemBus.isConnected()
        && systemBus.interface()
                   ->isServiceRegistered(QStringLiteral("org.freedesktop.RealtimeKit1"))
                   .value()) {
        qCDebug(lcAudioPort()) << "realtime endpoint: picking org.freedesktop.RealtimeKit1";
        return RealtimeEndpoint{ systemBus, QStringLiteral("org.freedesktop.RealtimeKit1"),
                                 QStringLiteral("/org/freedesktop/RealtimeKit1"),
                                 QStringLiteral("org.freedesktop.RealtimeKit1") };
    }

    return std::nullopt;
}

bool rtkitRequestRealtime()
{
    const auto endpoint = findRealTimeEndpoint();
    if (!endpoint) {
        qCDebug(lcAudioPort) << "failed to acquire real time endpoint";
        return false;
    }

    QDBusInterface props(endpoint->service, endpoint->path,
                         QStringLiteral("org.freedesktop.DBus.Properties"), endpoint->bus);

    const auto readProperty = [&props, &endpoint](const char *name, qlonglong fallback) {
        const QDBusReply<QDBusVariant> reply =
                props.call(QStringLiteral("Get"), endpoint->interface, QString::fromLatin1(name));
        return reply.isValid() ? reply.value().variant().toLongLong() : fallback;
    };

    const auto maxPriority = readProperty("MaxRealtimePriority", MEDIA_THREAD_RT_PRIORITY);
    const auto maxRtTime = readProperty("RTTimeUSecMax", MEDIA_THREAD_RTTIME_USEC);

    if (maxPriority <= 0) {
        qCDebug(lcAudioPort) << "no real time priority available thru RealtimeKit";
        return false;
    }

    const rlim_t rtTime =
            static_cast<rlim_t>(std::min<qlonglong>(maxRtTime, MEDIA_THREAD_RTTIME_USEC));
    struct rlimit limit = { rtTime, rtTime };
    if (setrlimit(RLIMIT_RTTIME, &limit) != 0) {
        qCWarning(lcAudioPort) << "unable to set RLIMIT_RTTIME, skipping realtime priority";
        return false;
    }

    const auto priority =
            static_cast<quint32>(std::min<qlonglong>(maxPriority, MEDIA_THREAD_RT_PRIORITY));
    const auto pid = static_cast<quint64>(getpid());
    const auto tid = static_cast<quint64>(syscall(SYS_gettid));

    QDBusInterface rtkit(endpoint->service, endpoint->path, endpoint->interface, endpoint->bus);

    const QDBusReply<void> reply =
            rtkit.call(QStringLiteral("MakeThreadRealtimeWithPID"), QVariant::fromValue(pid),
                       QVariant::fromValue(tid), QVariant::fromValue(priority));
    if (!reply.isValid()) {
        qCDebug(lcAudioPort).nospace() << "RealtimeKit refused thread " << tid << " of process "
                                       << pid << ": " << reply.error().message();
        return false;
    }

    qCDebug(lcAudioPort) << "media thread" << tid << "is now realtime at priority" << priority;
    return true;
}
#endif

void raiseCurrentThreadPriority()
{
#ifdef Q_OS_LINUX
    if (rtkitRequestRealtime()) {
        return;
    }
#endif

    // Fall back to pjsip mechanism for the current platform
    pj_thread_t *thisThread = pj_thread_this();
    if (!thisThread) {
        return;
    }

    const int maxPriority = pj_thread_get_prio_max(thisThread);
    if (maxPriority <= 0) {
        qCDebug(lcAudioPort) << "no raised priority available for the media thread";
        return;
    }

    if (pj_thread_set_prio(thisThread, maxPriority) != PJ_SUCCESS) {
        qCDebug(lcAudioPort) << "unable to raise media thread priority to" << maxPriority;
    }
}

void ensureMediaThreadPriority()
{
    static thread_local bool guard = false;
    if (!guard) {
        guard = true;
        raiseCurrentThreadPriority();
    }
}

} // namespace

AudioPort::AudioPort(QAudioDevice device) : m_device(device)
{
    m_idleTimer.setInterval(10s);
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

    if (m_device.mode() == QAudioDevice::Mode::Input) {
        try {
            adjustTxLevel(activeTxLevel());
        } catch (pj::Error &err) {
            qCCritical(lcAudioPort) << "failed to adjust tx level: " << err.info();
        }
    }

    return true;
}

float AudioPort::activeTxLevel() const
{
    // Fixed level or AGC?
    if (m_audioProcessor && m_audioProcessor->hasGainControl()) {
        return 1.0f;
    }
    return NORMAL_AUDIO_LEVEL;
}

void AudioPort::setAudioProcessor(AudioProcessor *audioProcessor)
{
    m_audioProcessor = audioProcessor;

    if (audioProcessor && m_device.mode() == QAudioDevice::Mode::Input && !m_isMuted) {
        try {
            adjustTxLevel(activeTxLevel());
        } catch (pj::Error &err) {
            qCCritical(lcAudioPort) << "failed to adjust tx level: " << err.info();
        }
    }
}

void AudioPort::setMuted(bool value)
{
    if (m_isMuted != value) {
        if (m_device.mode() == QAudioDevice::Mode::Input) {
            try {
                adjustTxLevel(value ? 0.0f : activeTxLevel());
            } catch (pj::Error &err) {
                qCCritical(lcAudioPort) << "failed to adjust tx level: " << err.info();
            }
        }

        m_isMuted = value;
    }
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
    qreal max = 0.0;
    const qint64 numSamples = size / sizeof(qint16);

    static constexpr qreal positiveRange = std::numeric_limits<qint16>::max();

    for (int i = 0; i < numSamples; ++i) { // i is index of sample
        const qint16 sample = qFromLittleEndian<qint16>(data);
        const qreal level = qAbs(static_cast<qreal>(sample)) / positiveRange;

        max = std::max(max, level);
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

void AudioPort::requestStartIO()
{
    if (m_ioStartRequested.exchange(true)) {
        return;
    }

    QMetaObject::invokeMethod(
            this,
            [this]() {
                startIO();
                m_ioStartRequested = false;
            },
            Qt::QueuedConnection);
}

void AudioPort::startIO()
{
    const QMutexLocker locker(&m_ioMutex);
    startIONoLock();
}

void AudioPort::startIONoLock()
{
    if (m_device.mode() == QAudioDevice::Mode::Input) {
        startSourceIONoLock();
    } else {
        startSinkIONoLock();
    }
}

void AudioPort::stopIO()
{
    const QMutexLocker locker(&m_ioMutex);
    stopIONoLock();
}

void AudioPort::stopIONoLock()
{
    if (m_device.mode() == QAudioDevice::Mode::Input) {
        stopSourceIONoLock();
    } else {
        stopSinkIONoLock();
    }
}

void AudioPort::writeSilenceMSNoLock(unsigned milliseconds)
{
    if (m_sink) {
        if (!m_io.isNull()) {
            pj::MediaFormatAudio fmt;
            try {
                fmt = getPortInfo().format;
            } catch (pj::Error &err) {
                qCWarning(lcAudioPort)
                        << "failed to get port info for silence write: " << err.info();
                return;
            }
            unsigned byteCount =
                    (fmt.clockRate * fmt.channelCount * (fmt.bitsPerSample / 8) * milliseconds)
                    / 1000;

            // Write silence to allow USB headsets to switch audio mode without
            // ugly crackling noise.
            QByteArray silence(byteCount, 0);
            const qint64 written = m_io->write(silence);

            qCDebug(lcAudioPort).nospace()
                    << "silence write: " << written << " of " << byteCount << " bytes ("
                    << milliseconds << "ms @ " << fmt.clockRate << "Hz/" << fmt.channelCount
                    << "ch/" << fmt.bitsPerSample
                    << "bit), sink bufferSize=" << m_sink->bufferSize()
                    << ", bytesFree=" << m_sink->bytesFree();
        }
    }
}

bool AudioPort::isSilence(const void *data, unsigned size) const
{
    // ~1% of full scale - comfort noise and DC offsets stay below this
    static constexpr int threshold = 328;

    const auto *bytes = static_cast<const char *>(data);
    const unsigned samples = size / sizeof(qint16);

    for (unsigned i = 0; i < samples; ++i) {
        const auto sample = qFromLittleEndian<qint16>(bytes + i * sizeof(qint16));
        if (qAbs(static_cast<int>(sample)) > threshold) {
            return false;
        }
    }

    return true;
}

void AudioPort::padSilence()
{
    const QMutexLocker locker(&m_ioMutex);
    padSilenceNoLock();
}

void AudioPort::padSilenceNoLock()
{
    if (m_sink.isNull() || m_io.isNull()) {
        return;
    }

    // Note: the fill level has to be computed from the sink format, not from the pjsip
    // port format - only the sink tells us how many bytes are still queued for playback.
    const qint64 target = m_audioFormat.bytesForDuration(SILENCE_TARGET_FILL_MS * 1000);
    const qint64 queued = m_sink->bufferSize() - m_sink->bytesFree();
    const qint64 missing = target - queued;

    if (missing <= 0) {
        qCDebug(lcAudioPort).nospace() << "silence pad: nothing written, buffer already holds "
                                       << (m_audioFormat.durationForBytes(queued) / 1000)
                                       << "ms of " << SILENCE_TARGET_FILL_MS << "ms target";
        return;
    }

    const qint64 written = m_io->write(QByteArray(missing, 0));

    qCDebug(lcAudioPort).nospace()
            << "silence pad: wrote " << written << " of " << missing << " bytes ("
            << (m_audioFormat.durationForBytes(written) / 1000) << "ms) to reach the "
            << SILENCE_TARGET_FILL_MS << "ms target, buffer was at "
            << (m_audioFormat.durationForBytes(queued) / 1000) << "ms";
}

void AudioPort::startSinkIONoLock()
{
    m_idleTimer.stop();

    if (m_isDraining && !m_sink.isNull()) {
        m_isDraining = false;
        return;
    }

    if (!m_sink.isNull()) {
        stopSinkIONoLock();
    }

    qCInfo(lcAudioPort).noquote().nospace()
            << "Initialize sink of device_descr=\"" << m_device.description() << "\", device_id=\""
            << m_device.id()
            << "\", with settings:" << "\nsampleRate=" << m_audioFormat.sampleRate()
            << "\nchannelCount=" << m_audioFormat.channelCount()
            << "\nbytesPerSample=" << m_audioFormat.bytesPerSample()
            << "\nsampleFormat=" << m_audioFormat.sampleFormat();

    m_sink = new QAudioSink(m_device, m_audioFormat);
    m_io = m_sink->start();

    padSilenceNoLock();

    Q_EMIT audioSinkChanged();
}

void AudioPort::stopSinkIONoLock()
{
    m_idleTimer.stop();
    if (m_sink.isNull() || m_isDraining) {
        return;
    }

    writeSilenceMSNoLock(SILENCE_BUFFER_MS);

    m_isDraining = true;
    QTimer::singleShot(SILENCE_BUFFER_MS, this, [this]() {
        {
            const QMutexLocker locker(&m_ioMutex);

            m_isDraining = false;

            if (m_sink) {
                m_sink->stop();
                m_sink->deleteLater();
                m_sink = nullptr;
                m_io = nullptr;
            }
        }
        Q_EMIT audioSinkChanged();
    });
}

void AudioPort::startSourceIONoLock()
{
    m_idleTimer.stop();

    if (!m_source.isNull()) {
        stopSourceIONoLock();
        delete m_source;
        m_source = nullptr;
    }

    qCInfo(lcAudioPort).noquote().nospace()
            << "Initialize source of device_descr=\"" << m_device.description()
            << "\", device_id=\"" << m_device.id()
            << "\", with settings:" << "\nsampleRate=" << m_audioFormat.sampleRate()
            << "\nchannelCount=" << m_audioFormat.channelCount()
            << "\nbytesPerSample=" << m_audioFormat.bytesPerSample()
            << "\nsampleFormat=" << m_audioFormat.sampleFormat();

    m_source = new QAudioSource(m_device, m_audioFormat);
    m_io = m_source->start();

    Q_EMIT audioSourceChanged();
}

void AudioPort::stopSourceIONoLock()
{
    m_idleTimer.stop();
    if (m_source) {
        m_source->stop();
        m_source->deleteLater();
        m_source = nullptr;
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
    ensureMediaThreadPriority();

    if (m_device.mode() != QAudioDevice::Mode::Input) {
        return;
    }

    // Skip when locked
    std::unique_lock<QMutex> locker(m_ioMutex, std::try_to_lock);
    if (!locker.owns_lock()) {
        return;
    }

    // Create source on the fly if needed
    if (m_source.isNull()) {
        requestStartIO();
    }

    if (m_io.isNull()) {
        qCWarning(lcAudioPort) << "frame requested, but no IO available";
        return;
    }

    if (m_isWarmingUp) {
        m_isWarmingUp = false;
        QObject::disconnect(m_warmUpDrain);
    }

    auto bytes = m_io->read(frame.size);

    if (++m_captureFrames % 50 == 0 && !m_source.isNull()) {
        const auto queued = m_source->bytesAvailable();
        qCDebug(lcAudioPort).nospace()
                << "capture: read " << bytes.size() << " of " << frame.size
                << " bytes, source bufferSize=" << m_source->bufferSize() << ", queued=" << queued
                << " bytes (" << (m_audioFormat.durationForBytes(queued) / 1000)
                << "ms), processor=" << (m_audioProcessor ? "on" : "off");
    }

    if (!m_isMuted) {
        frame.buf = std::vector<unsigned char>(bytes.constBegin(), bytes.constEnd());
        frame.type = PJMEDIA_FRAME_TYPE_AUDIO;

        // Optionally apply AGC/ANC/AEC
        if (m_audioProcessor) {
            m_audioProcessor->capture(frame.buf.data(), static_cast<unsigned>(frame.buf.size()));
        }

        updateAudioLevel(reinterpret_cast<const char *>(frame.buf.data()), frame.buf.size());
    } else {
        setSourceAudioLevel(0);
    }

    // Auto destroy sink after timeout
    Q_EMIT startIdleTimer();
}

void AudioPort::onFrameReceived(pj::MediaFrame &frame)
{
    ensureMediaThreadPriority();

    if (m_device.mode() == QAudioDevice::Mode::Input) {
        return;
    }

    // Skip when locked
    std::unique_lock<QMutex> locker(m_ioMutex, std::try_to_lock);
    if (!locker.owns_lock()) {
        return;
    }

    // Create sink on the fly if needed
    if (m_sink.isNull()) {
        requestStartIO();
    }

    if (m_io.isNull()) {
        qCWarning(lcAudioPort) << "frame received, but no IO available";
        return;
    }

    m_isWarmingUp = false;

    // Register the frame about to be played as the echo reference.
    if (m_audioProcessor) {
        m_audioProcessor->playback(frame.buf.data(), static_cast<unsigned>(frame.size));
    }

    // Resolve any excess buffering by skipping frames that carry silence anyway
    const qint64 target = m_audioFormat.bytesForDuration(SILENCE_TARGET_FILL_MS * 1000);
    if (!m_sink.isNull() && m_sink->bufferSize() - m_sink->bytesFree() > target
        && isSilence(frame.buf.data(), static_cast<unsigned>(frame.size))) {

        qCDebug(lcAudioPort).nospace()
                << "playback: dropped a silent frame to drain "
                << (m_audioFormat.durationForBytes(m_sink->bufferSize() - m_sink->bytesFree())
                    / 1000)
                << "ms down to the " << SILENCE_TARGET_FILL_MS << "ms target";

        Q_EMIT startIdleTimer();
        return;
    }

    m_io->write(reinterpret_cast<char *>(frame.buf.data()), frame.size);

    // Auto destroy sink after timeout
    Q_EMIT startIdleTimer();
}

void AudioPort::acquire()
{
    const QMutexLocker locker(&m_ioMutex);

    if (m_device.mode() == QAudioDevice::Mode::Input) {
        if (m_source.isNull()) {
            startSourceIONoLock();
        }
    } else {
        if (m_sink.isNull()) {
            startSinkIONoLock();
        }
    }

    m_isWarmingUp = true;

    if (m_device.mode() == QAudioDevice::Mode::Input && !m_io.isNull()) {
        QObject::disconnect(m_warmUpDrain);

        m_warmUpDrain = connect(m_io.data(), &QIODevice::readyRead, this, [this]() {
            if (m_isWarmingUp && !m_io.isNull()) {
                m_io->readAll();
            }
        });
    }
}

void AudioPort::release()
{
    if (!m_isWarmingUp) {
        return;
    }

    m_isWarmingUp = false;

    QObject::disconnect(m_warmUpDrain);

    if (!m_idleTimer.isActive()) {
        stopIO();
    }
}
