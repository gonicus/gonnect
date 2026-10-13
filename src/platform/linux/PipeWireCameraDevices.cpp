#include <fcntl.h>
#include <QLoggingCategory>
#include "PipeWireCameraDevices.h"

using namespace std::chrono_literals;

Q_LOGGING_CATEGORY(lcPwCamera, "gonnect.media.pipewire.camera")

PipeWireCameraDevices::PipeWireCameraDevices(QObject *parent) : QObject(parent)
{
    connect(&m_iterateTimer, &QTimer::timeout, this, &PipeWireCameraDevices::iterate);
}

PipeWireCameraDevices::~PipeWireCameraDevices()
{
    stop();
}

bool PipeWireCameraDevices::start(int fd)
{
    if (fd < 0) {
        return false;
    }

    stop();

    pw_init(nullptr, nullptr);

    m_fd = fd;
    m_mainLoop = pw_main_loop_new(nullptr);
    m_loop = pw_main_loop_get_loop(m_mainLoop);
    m_context = pw_context_new(m_loop, nullptr, 0);

    if (!m_context) {
        qCWarning(lcPwCamera) << "failed to create PipeWire context";
        stop();
        return false;
    }

    // pw_context_connect_fd() takes ownership of the descriptor, keep a copy here
    const int connectFd = ::fcntl(m_fd, F_DUPFD_CLOEXEC, 3);

    if (connectFd < 0) {
        qCWarning(lcPwCamera) << "could not duplicate fd:" << std::strerror(errno);
        stop();
        return false;
    }

    m_core = pw_context_connect_fd(m_context, connectFd, nullptr, 0);

    if (!m_core) {
        qCWarning(lcPwCamera) << "could not connect to PipeWire remote:" << std::strerror(errno);
        stop();
        return false;
    }

    m_registry = pw_core_get_registry(m_core, PW_VERSION_REGISTRY, 0);

    static const pw_registry_events registryEvents = {
        .version = PW_VERSION_REGISTRY_EVENTS,
        .global = registryGlobal,
        .global_remove = registryGlobalRemove,
    };
    pw_registry_add_listener(m_registry, &m_registryListener, &registryEvents, this);

    m_iterateTimer.start(100ms);

    return true;
}

void PipeWireCameraDevices::stop()
{
    m_iterateTimer.stop();

    if (m_registry) {
        spa_hook_remove(&m_registryListener);
        pw_proxy_destroy(reinterpret_cast<struct pw_proxy *>(m_registry));
        m_registry = nullptr;
    }

    if (m_core) {
        pw_core_disconnect(m_core);
        m_core = nullptr;
    }

    if (m_context) {
        pw_context_destroy(m_context);
        m_context = nullptr;
    }

    if (m_mainLoop) {
        pw_main_loop_destroy(m_mainLoop);
        m_mainLoop = nullptr;
    }

    m_loop = nullptr;

    if (m_fd >= 0) {
        ::close(m_fd);
        m_fd = -1;
    }

    if (!m_devices.isEmpty()) {
        m_devices.clear();
        Q_EMIT devicesChanged();
    }

    pw_deinit();
}

void PipeWireCameraDevices::registryGlobal(void *data, uint32_t id, uint32_t permissions,
                                           const char *type, uint32_t version,
                                           const struct spa_dict *props)
{
    Q_UNUSED(permissions)
    Q_UNUSED(version)

    auto self = static_cast<PipeWireCameraDevices *>(data);

    if (!props || strcmp(type, PW_TYPE_INTERFACE_Node) != 0) {
        return;
    }

    // Decision "what is a camera" is taken from the xdg-desktop-portal mechanism,
    // so the portal and us should agree on that
    const char *mediaClass = spa_dict_lookup(props, PW_KEY_MEDIA_CLASS);
    if (!mediaClass || strcmp(mediaClass, "Video/Source") != 0) {
        return;
    }

    const char *mediaRole = spa_dict_lookup(props, PW_KEY_MEDIA_ROLE);
    if (!mediaRole || strcmp(mediaRole, "Camera") != 0) {
        return;
    }

    PipeWireCameraDevice device;
    device.nodeId = id;

    if (const char *serial = spa_dict_lookup(props, PW_KEY_OBJECT_SERIAL)) {
        device.serial = QString::fromUtf8(serial).toULongLong();
    }
    if (const char *name = spa_dict_lookup(props, PW_KEY_NODE_NAME)) {
        device.name = QString::fromUtf8(name);
    }
    if (const char *description = spa_dict_lookup(props, PW_KEY_NODE_DESCRIPTION)) {
        device.description = QString::fromUtf8(description);
    }

    // Don't produce empty descriptions for the UI
    if (device.description.isEmpty()) {
        device.description = device.name;
    }

    if (!self->m_devices.contains(device)) {
        qCInfo(lcPwCamera) << "camera found:" << device.nodeId << device.name << device.description;
        self->m_devices.append(device);
        Q_EMIT self->devicesChanged();
    }
}

void PipeWireCameraDevices::registryGlobalRemove(void *data, uint32_t id)
{
    auto self = static_cast<PipeWireCameraDevices *>(data);

    const auto removed = self->m_devices.removeIf(
            [id](const PipeWireCameraDevice &device) { return device.nodeId == id; });

    if (removed > 0) {
        qCInfo(lcPwCamera) << "camera removed:" << id;
        Q_EMIT self->devicesChanged();
    }
}

void PipeWireCameraDevices::iterate()
{
    if (m_loop) {
        pw_loop_iterate(m_loop, 0);
    }
}
