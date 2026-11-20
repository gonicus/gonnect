#pragma once

#include "CameraSource.h"
#include "PipeWireFrameConverter.h"

#include <QMap>

#include <pipewire/pipewire.h>

#include <atomic>
#include <optional>

/// Captures camera frames from PipeWire.
/// Frames and signals are emitted on own thread.
class PipeWireCameraSource : public CameraSource
{
    Q_OBJECT

public:
    explicit PipeWireCameraSource(QObject *parent = nullptr);
    ~PipeWireCameraSource() override;

    /// Connects to the remote (via fd or PipeWire daemon) and takes ownership
    bool open(int fd);
    void close();
    bool isOpen() const { return m_core != nullptr; }

    QList<VideoDevice> devices() const override;
    void start(const QString &deviceId) override;
    void stop() override;
    bool isActive() const override { return m_active; }

private:
    static void onCoreError(void *data, uint32_t id, int seq, int res, const char *message);
    static void onRegistryGlobal(void *data, uint32_t id, uint32_t permissions, const char *type,
                                 uint32_t version, const spa_dict *props);
    static void onRegistryGlobalRemove(void *data, uint32_t id);
    static void onStreamStateChanged(void *data, pw_stream_state old, pw_stream_state state,
                                     const char *error);
    static void onStreamParamChanged(void *data, uint32_t id, const spa_pod *param);
    static void onStreamProcess(void *data);

    // Must be called with locked thread loop
    void createStream(const QString &deviceId);
    void destroyStream();
    void setActive(bool active);

    /// Returns description of format or nothing if not supported
    std::optional<QString> setConverterFormat(const spa_pod *format);

    pw_thread_loop *m_threadLoop = nullptr;
    pw_context *m_context = nullptr;
    pw_core *m_core = nullptr;
    pw_registry *m_registry = nullptr;
    pw_stream *m_stream = nullptr;
    spa_hook m_coreListener = {};
    spa_hook m_registryListener = {};
    spa_hook m_streamListener = {};

    // Only access when thread loop locked or stopped
    // node id -> camera
    QMap<uint32_t, VideoDevice> m_cameras;
    PipeWireFrameConverter m_converter;

    std::atomic<bool> m_active = false;
};
