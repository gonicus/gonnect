#include "PipeWireCameraSource.h"

#include <QLoggingCategory>

#include <spa/param/buffers.h>
#include <spa/param/video/format-utils.h>
#include <spa/pod/builder.h>

#include <array>
#include <cerrno>
#include <cstring>
#include <unistd.h>
#include <vector>

Q_LOGGING_CATEGORY(lcPipeWireCameraSource, "gonnect.media.camera.pipewire")

namespace {

// Same notion of camera as in camera portal
bool isCamera(const char *type, const spa_dict *props)
{
    if (!props || std::strcmp(type, PW_TYPE_INTERFACE_Node) != 0) {
        return false;
    }

    const char *mediaClass = spa_dict_lookup(props, PW_KEY_MEDIA_CLASS);
    const char *mediaRole = spa_dict_lookup(props, PW_KEY_MEDIA_ROLE);

    return mediaClass && mediaRole && std::strcmp(mediaClass, "Video/Source") == 0
            && std::strcmp(mediaRole, "Camera") == 0;
}

QString formatToString(const QString &name, const spa_rectangle &size,
                       const spa_fraction &framerate)
{
    const QString rate = framerate.denom > 0
            ? QString::number(static_cast<double>(framerate.num) / framerate.denom, 'f', 1)
            : QStringLiteral("variable");

    return QStringLiteral("%1 %2x%3 @ %4 fps").arg(name).arg(size.width).arg(size.height).arg(rate);
}

} // namespace

PipeWireCameraSource::PipeWireCameraSource(QObject *parent) : CameraSource{ parent }
{
    pw_init(nullptr, nullptr);
}

PipeWireCameraSource::~PipeWireCameraSource()
{
    close();
    pw_deinit();
}

bool PipeWireCameraSource::open(int fd)
{
    close();

    m_threadLoop = pw_thread_loop_new("gonnect-camera", nullptr);
    if (m_threadLoop) {
        m_context = pw_context_new(pw_thread_loop_get_loop(m_threadLoop), nullptr, 0);
    }

    if (!m_context || pw_thread_loop_start(m_threadLoop) < 0) {
        qCCritical(lcPipeWireCameraSource) << "Cannot start PipeWire thread loop";
        if (fd >= 0) {
            ::close(fd);
        }
        close();
        return false;
    }

    pw_thread_loop_lock(m_threadLoop);

    // Take ownership
    m_core = fd >= 0 ? pw_context_connect_fd(m_context, fd, nullptr, 0)
                     : pw_context_connect(m_context, nullptr, 0);

    if (!m_core) {
        pw_thread_loop_unlock(m_threadLoop);
        qCCritical(lcPipeWireCameraSource) << "Cannot connect to PipeWire:" << std::strerror(errno);
        close();
        return false;
    }

    static constexpr pw_core_events coreEvents = [] {
        pw_core_events events = {};
        events.version = PW_VERSION_CORE_EVENTS, events.error = onCoreError;
        return events;
    }();
    pw_core_add_listener(m_core, &m_coreListener, &coreEvents, this);

    static constexpr pw_registry_events registryEvents = [] {
        pw_registry_events events = {};
        events.version = PW_VERSION_REGISTRY_EVENTS;
        events.global = onRegistryGlobal;
        events.global_remove = onRegistryGlobalRemove;
        return events;
    }();
    m_registry = pw_core_get_registry(m_core, PW_VERSION_REGISTRY, 0);
    pw_registry_add_listener(m_registry, &m_registryListener, &registryEvents, this);

    pw_thread_loop_unlock(m_threadLoop);

    qCInfo(lcPipeWireCameraSource)
            << "Connected to PipeWire" << (fd >= 0 ? "via camera portal" : "directly");
    return true;
}

void PipeWireCameraSource::close()
{
    if (m_threadLoop) {
        pw_thread_loop_lock(m_threadLoop);

        destroyStream();

        if (m_registry) {
            spa_hook_remove(&m_registryListener);
            pw_proxy_destroy(reinterpret_cast<pw_proxy *>(m_registry));
            m_registry = nullptr;
        }

        if (m_core) {
            spa_hook_remove(&m_coreListener);
            pw_core_disconnect(m_core);
            m_core = nullptr;
        }

        pw_thread_loop_unlock(m_threadLoop);
        pw_thread_loop_stop(m_threadLoop);
    }

    if (m_context) {
        pw_context_destroy(m_context);
        m_context = nullptr;
    }

    if (m_threadLoop) {
        pw_thread_loop_destroy(m_threadLoop);
        m_threadLoop = nullptr;
    }

    if (!m_cameras.isEmpty()) {
        m_cameras.clear();
        Q_EMIT devicesChanged();
    }
}

QList<VideoDevice> PipeWireCameraSource::devices() const
{
    if (!isOpen()) {
        return {};
    }

    pw_thread_loop_lock(m_threadLoop);
    QList<VideoDevice> devices = m_cameras.values();
    pw_thread_loop_unlock(m_threadLoop);

    // PipeWire has no default - use first
    if (!devices.isEmpty()) {
        devices.first().isDefault = true;
    }

    return devices;
}

void PipeWireCameraSource::start(const QString &deviceId)
{
    if (!isOpen()) {
        qCCritical(lcPipeWireCameraSource) << "Cannot start camera - no PipeWire connection";
        Q_EMIT errorOccurred(tr("Camera is not available"));
        return;
    }

    pw_thread_loop_lock(m_threadLoop);
    destroyStream();
    createStream(deviceId);
    pw_thread_loop_unlock(m_threadLoop);
}

void PipeWireCameraSource::stop()
{
    if (!isOpen()) {
        return;
    }

    pw_thread_loop_lock(m_threadLoop);
    destroyStream();
    pw_thread_loop_unlock(m_threadLoop);
}

void PipeWireCameraSource::onCoreError(void *data, uint32_t id, int seq, int res,
                                       const char *message)
{
    Q_UNUSED(seq)

    qCCritical(lcPipeWireCameraSource)
            << "Core error on object" << id << ":" << message << std::strerror(-res);

    if (id == PW_ID_CORE) {
        Q_EMIT static_cast<PipeWireCameraSource *>(data)->errorOccurred(QString::fromUtf8(message));
    }
}

void PipeWireCameraSource::onRegistryGlobal(void *data, uint32_t id, uint32_t permissions,
                                            const char *type, uint32_t version,
                                            const spa_dict *props)
{
    Q_UNUSED(permissions)
    Q_UNUSED(version)

    if (!isCamera(type, props)) {
        return;
    }

    const char *name = spa_dict_lookup(props, PW_KEY_NODE_NAME);
    const char *description = spa_dict_lookup(props, PW_KEY_NODE_DESCRIPTION);
    const VideoDevice device{ QString::fromUtf8(name),
                              QString::fromUtf8(description ? description : name), false };

    // Node may be announced multiple times
    auto *self = static_cast<PipeWireCameraSource *>(data);
    self->m_cameras.insert(id, device);

    qCInfo(lcPipeWireCameraSource) << "Camera found:" << id << device.id << device.description;
    Q_EMIT self->devicesChanged();
}

void PipeWireCameraSource::onRegistryGlobalRemove(void *data, uint32_t id)
{
    auto *self = static_cast<PipeWireCameraSource *>(data);
    if (self->m_cameras.remove(id) > 0) {
        qCInfo(lcPipeWireCameraSource) << "Camera removed:" << id;
        Q_EMIT self->devicesChanged();
    }
}

void PipeWireCameraSource::onStreamStateChanged(void *data, pw_stream_state old,
                                                pw_stream_state state, const char *error)
{
    qCInfo(lcPipeWireCameraSource) << "Stream state" << pw_stream_state_as_string(old) << "->"
                                   << pw_stream_state_as_string(state) << (error ? error : "");

    auto *self = static_cast<PipeWireCameraSource *>(data);
    self->setActive(state == PW_STREAM_STATE_STREAMING);

    if (state == PW_STREAM_STATE_ERROR) {
        Q_EMIT self->errorOccurred(QString::fromUtf8(error));
    }
}

void PipeWireCameraSource::onStreamParamChanged(void *data, uint32_t id, const spa_pod *param)
{
    if (!param || id != SPA_PARAM_Format) {
        return;
    }

    auto *self = static_cast<PipeWireCameraSource *>(data);
    const auto description = self->setConverterFormat(param);
    if (!description) {
        return;
    }

    qCInfo(lcPipeWireCameraSource) << "Negotiated format:" << *description;
    Q_EMIT self->formatChanged(*description);

    std::array<uint8_t, 256> podBuffer;
    spa_pod_builder builder;
    spa_pod_builder_init(&builder, podBuffer.data(), static_cast<uint32_t>(podBuffer.size()));

    spa_pod_frame frame;
    spa_pod_builder_push_object(&builder, &frame, SPA_TYPE_OBJECT_ParamBuffers, SPA_PARAM_Buffers);
    spa_pod_builder_add(&builder, SPA_PARAM_BUFFERS_dataType,
                        SPA_POD_CHOICE_FLAGS_Int((1 << SPA_DATA_MemPtr) | (1 << SPA_DATA_MemFd)),
                        0); // Sentinel
    const auto *buffers = static_cast<const spa_pod *>(spa_pod_builder_pop(&builder, &frame));

    std::array<const spa_pod *, 1> params = { buffers };

    pw_stream_update_params(self->m_stream, params.data(), static_cast<uint32_t>(params.size()));
}

void PipeWireCameraSource::onStreamProcess(void *data)
{
    auto *self = static_cast<PipeWireCameraSource *>(data);

    pw_buffer *buffer = pw_stream_dequeue_buffer(self->m_stream);
    if (!buffer) {
        return;
    }

    const std::optional<QVideoFrame> frame = self->m_converter.convert(*buffer->buffer);
    pw_stream_queue_buffer(self->m_stream, buffer);

    if (frame) {
        self->deliverFrame(*frame);
    }
}

void PipeWireCameraSource::createStream(const QString &deviceId)
{
    pw_properties *properties = pw_properties_new(PW_KEY_MEDIA_TYPE, "Video", PW_KEY_MEDIA_CATEGORY,
                                                  "Capture", PW_KEY_MEDIA_ROLE, "Camera", nullptr);

    if (!deviceId.isEmpty()) {
        pw_properties_set(properties, PW_KEY_TARGET_OBJECT, deviceId.toUtf8().constData());
    }

    // Take ownership of properties
    m_stream = pw_stream_new(m_core, "gonnect-camera", properties);
    if (!m_stream) {
        qCCritical(lcPipeWireCameraSource) << "Cannot create stream:" << std::strerror(errno);
        return;
    }

    static constexpr pw_stream_events streamEvents = [] {
        pw_stream_events events = {};
        events.version = PW_VERSION_STREAM_EVENTS;
        events.state_changed = onStreamStateChanged;
        events.param_changed = onStreamParamChanged;
        events.process = onStreamProcess;
        return events;
    }();
    pw_stream_add_listener(m_stream, &m_streamListener, &streamEvents, this);

    spa_rectangle defaultSize = { 1280, 720 };
    spa_rectangle minSize = { 1, 1 };
    spa_rectangle maxSize = { 4096, 4096 };
    spa_fraction defaultRate = { 30, 1 };
    spa_fraction minRate = { 0, 1 };
    spa_fraction maxRate = { 120, 1 };

    std::array<uint8_t, 1024> podBuffer;
    spa_pod_builder builder;
    spa_pod_builder_init(&builder, podBuffer.data(), static_cast<uint32_t>(podBuffer.size()));

    // First offer: many cameras reach 720p30 only compressed
    spa_pod_frame frame;
    spa_pod_builder_push_object(&builder, &frame, SPA_TYPE_OBJECT_Format, SPA_PARAM_EnumFormat);
    spa_pod_builder_add(&builder, SPA_FORMAT_mediaType, SPA_POD_Id(SPA_MEDIA_TYPE_video),
                        SPA_FORMAT_mediaSubtype, SPA_POD_Id(SPA_MEDIA_SUBTYPE_mjpg),
                        SPA_FORMAT_VIDEO_size,
                        SPA_POD_CHOICE_RANGE_Rectangle(&defaultSize, &minSize, &maxSize),
                        SPA_FORMAT_VIDEO_framerate,
                        SPA_POD_CHOICE_RANGE_Fraction(&defaultRate, &minRate, &maxRate), 0);
    const auto *mjpegFormat = static_cast<const spa_pod *>(spa_pod_builder_pop(&builder, &frame));

    spa_pod_builder_push_object(&builder, &frame, SPA_TYPE_OBJECT_Format, SPA_PARAM_EnumFormat);
    spa_pod_builder_add(
            &builder, SPA_FORMAT_mediaType, SPA_POD_Id(SPA_MEDIA_TYPE_video),
            SPA_FORMAT_mediaSubtype, SPA_POD_Id(SPA_MEDIA_SUBTYPE_raw), SPA_FORMAT_VIDEO_format,
            SPA_POD_CHOICE_ENUM_Id(6, SPA_VIDEO_FORMAT_YUY2, SPA_VIDEO_FORMAT_YUY2,
                                   SPA_VIDEO_FORMAT_NV12, SPA_VIDEO_FORMAT_I420,
                                   SPA_VIDEO_FORMAT_BGRx, SPA_VIDEO_FORMAT_GRAY8),
            SPA_FORMAT_VIDEO_size, SPA_POD_CHOICE_RANGE_Rectangle(&defaultSize, &minSize, &maxSize),
            SPA_FORMAT_VIDEO_framerate,
            SPA_POD_CHOICE_RANGE_Fraction(&defaultRate, &minRate, &maxRate), 0);
    const auto *rawFormat = static_cast<const spa_pod *>(spa_pod_builder_pop(&builder, &frame));

    std::vector<const spa_pod *> params;
    if (PipeWireFrameConverter::isMjpegSupported()) {
        params.push_back(mjpegFormat);
    } else {
        qCWarning(lcPipeWireCameraSource) << "No MJPEG decoder - offering raw formats only";
    }
    params.push_back(rawFormat);

    // Prevent false positive: the value is not a dedicated member of the enum, but the value is
    // in range and therefore valid.
    // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
    const auto flags =
            static_cast<pw_stream_flags>(PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS);
    const int result = pw_stream_connect(m_stream, PW_DIRECTION_INPUT, PW_ID_ANY, flags,
                                         params.data(), static_cast<uint32_t>(params.size()));

    if (result < 0) {
        qCCritical(lcPipeWireCameraSource) << "Cannot connect stream:" << std::strerror(-result);
        destroyStream();
    }
}

void PipeWireCameraSource::destroyStream()
{
    if (!m_stream) {
        return;
    }

    spa_hook_remove(&m_streamListener);
    pw_stream_destroy(m_stream);
    m_stream = nullptr;
    setActive(false);
}

void PipeWireCameraSource::setActive(bool active)
{
    if (m_active.exchange(active) != active) {
        Q_EMIT activeChanged();
    }
}

std::optional<QString> PipeWireCameraSource::setConverterFormat(const spa_pod *format)
{
    uint32_t mediaType = 0;
    uint32_t mediaSubtype = 0;
    if (spa_format_parse(format, &mediaType, &mediaSubtype) < 0
        || mediaType != SPA_MEDIA_TYPE_video) {
        return std::nullopt;
    }

    if (mediaSubtype == SPA_MEDIA_SUBTYPE_mjpg) {
        spa_video_info_mjpg info = {};
        if (spa_format_video_mjpg_parse(format, &info) < 0 || !m_converter.setMjpegFormat()) {
            qCWarning(lcPipeWireCameraSource) << "Cannot decode MJPEG";
            return std::nullopt;
        }
        return formatToString(QStringLiteral("MJPEG"), info.size, info.framerate);
    }

    spa_video_info_raw info = {};
    if (spa_format_video_raw_parse(format, &info) < 0 || !m_converter.setFormat(info)) {
        qCWarning(lcPipeWireCameraSource)
                << "Unsupported format:" << PipeWireFrameConverter::formatName(info.format);
        return std::nullopt;
    }

    return formatToString(PipeWireFrameConverter::formatName(info.format), info.size,
                          info.framerate);
}
