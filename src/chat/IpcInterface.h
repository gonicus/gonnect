#pragma once

#include <QObject>
#include <QLocalSocket>
#include <QLocalServer>
#include <QProcess>
#include <QPointer>

class IpcInterface : public QObject
{
    Q_OBJECT

public:
    explicit IpcInterface(QObject *parent = nullptr);

    /// Set the path to the binary that shall be started as subprocess. Setting the path has no
    /// effect until start() is called.
    void setBinaryPath(const QString &path) { m_binaryPath = path; }

    /// Set the arguments that shell be forwarded to the binary. Setting the arguments has no effect
    /// until start() is called.
    void setBinaryArguments(const QStringList &args) { m_args = args; }

    /// Start the binary as a subprocess and initialize the IPC communication. When everything has
    /// been initialized and die IPC is ready, isRunning() will yield true and the signal
    /// isRunningChanged() will be emitted. If isRunning() does not become true, an error occurred.
    void start();

    /// Whether the subprocess is running and IPC has been successfully established.
    bool isRunning() const { return m_isRunning; }

    /// Send this data to the sub process.
    quint64 writeData(const QByteArray &data);

    /// Full name/path to the request server. The subprocess must know this. It is usually given to
    /// it via setBinaryArguments();
    QString fullRequestServerName() const;

    /// Full name/path to the response server. The subprocess must know this. It is usually given to
    /// it via setBinaryArguments();
    QString fullResponseServerName() const;

Q_SIGNALS:

    /// Called when the subprocess and the IPC has successfully started or ended.
    void isRunningChanged();

    /// Emitted when a blob has been received via IPC.
    void blobReceived(QByteArray data);

private Q_SLOTS:
    void onReadReady();

private:
    void enlargeBuffer(quint64 newTotalSize);
    void setIsRunning(bool value);

    QPointer<QLocalSocket> m_socketRequest;
    QPointer<QLocalSocket> m_socketResponse;
    QLocalServer m_localServerRequest;
    QLocalServer m_localServerResponse;
    QString m_binaryPath;
    QStringList m_args;

    bool m_isRunning = false;
    QProcess m_process;
    quint64 m_sizeBuffer = 0;
    quint64 m_currentBufferSize = 0;
    char *m_buffer = nullptr;
};
