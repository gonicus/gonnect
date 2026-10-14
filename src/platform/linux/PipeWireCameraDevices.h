#pragma once
#include <QObject>
#include <QTimer>
#include <QList>

#include <pipewire/pipewire.h>

struct PipeWireCameraDevice
{
    quint32 nodeId = 0;
    quint64 serial = 0;
    QString name;
    QString description;

    bool operator==(const PipeWireCameraDevice &other) const { return nodeId == other.nodeId; }
};

class PipeWireCameraDevices : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(PipeWireCameraDevices)

public:
    explicit PipeWireCameraDevices(QObject *parent = nullptr);
    ~PipeWireCameraDevices();

    // Connects to the fd provided by the CameraPortal
    bool start(int fd);
    void stop();

    bool isRunning() const { return m_core != nullptr; }

    QList<PipeWireCameraDevice> devices() const { return m_devices; }

Q_SIGNALS:
    void devicesChanged();

private:
    static void registryGlobal(void *data, uint32_t id, uint32_t permissions, const char *type,
                               uint32_t version, const struct spa_dict *props);
    static void registryGlobalRemove(void *data, uint32_t id);

    void iterate();

    int m_fd = -1;
    struct pw_main_loop *m_mainLoop = nullptr;
    struct pw_loop *m_loop = nullptr;
    struct pw_context *m_context = nullptr;
    struct pw_core *m_core = nullptr;
    struct pw_registry *m_registry = nullptr;
    struct spa_hook m_registryListener;

    bool m_alreadyStarted = false;

    QTimer m_iterateTimer;
    QList<PipeWireCameraDevice> m_devices;
};
