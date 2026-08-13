#include "IpcInterface.h"
#include <qendian.h>
#include <QLoggingCategory>
#include <QFileInfo>
#include <QUuid>

#define GONNECT_BUFFER_SIZE 8 * 1024

Q_LOGGING_CATEGORY(lcIpcInterface, "gonnect.app.ipc")

IpcInterface::IpcInterface(QObject *parent) : QObject{ parent }
{
    m_buffer = static_cast<char *>(malloc(GONNECT_BUFFER_SIZE * sizeof(char)));
    m_currentBufferSize = GONNECT_BUFFER_SIZE;

    m_localServerResponse.setMaxPendingConnections(1);
    m_localServerResponse.setSocketOptions(QLocalServer::SocketOption::UserAccessOption);

    m_localServerRequest.setMaxPendingConnections(1);
    m_localServerRequest.setSocketOptions(QLocalServer::SocketOption::UserAccessOption);

    connect(&m_localServerResponse, &QLocalServer::newConnection, this, [this]() {
        if (m_localServerResponse.hasPendingConnections()) {

            if (m_socketResponse) {
                qCCritical(lcIpcInterface)
                        << "Ignoring a second connection in pipeline. This might "
                           "be a security issue.";
                return;
            }

            m_socketResponse = QPointer(m_localServerResponse.nextPendingConnection());
            connect(m_socketResponse, &QIODevice::readyRead, this, &IpcInterface::onReadReady);

            connect(m_socketResponse, &QLocalSocket::errorOccurred, this,
                    [](const QLocalSocket::LocalSocketError err) {
                        qCCritical(lcIpcInterface)
                                << "An error occurred in the response socket:" << err;
                        qFatal();
                    });

            if (!m_socketRequest.isNull()) {
                setIsRunning(true);
            }
        }
    });

    connect(&m_localServerRequest, &QLocalServer::newConnection, this, [this]() {
        if (m_localServerRequest.hasPendingConnections()) {

            if (m_socketRequest) {
                qCCritical(lcIpcInterface)
                        << "Ignoring a second connection in pipeline. This might "
                           "be a security issue.";
                return;
            }

            m_socketRequest = QPointer(m_localServerRequest.nextPendingConnection());

            connect(m_socketRequest, &QLocalSocket::errorOccurred, this,
                    [](const QLocalSocket::LocalSocketError err) {
                        qCCritical(lcIpcInterface)
                                << "An error occurred in the request socket:" << err;
                        qFatal();
                    });

            if (!m_socketResponse.isNull()) {
                setIsRunning(true);
            }
        }
    });

    const QString uuid = QUuid::createUuid().toString(QUuid::StringFormat::WithoutBraces);
    const auto requestSocket = QString("gonnect-request-socket-%1").arg(uuid);
    const auto responseSocket = QString("gonnect-response-socket-%1").arg(uuid);

    if (!m_localServerResponse.listen(responseSocket)) {
        qFatal("Cannot listen on local server");
    }

    if (!m_localServerRequest.listen(requestSocket)) {
        qFatal("Cannot listen on local server");
    }

    qCInfo(lcIpcInterface).noquote()
            << "Started local socket servers at" << m_localServerResponse.fullServerName() << "and"
            << m_localServerRequest.fullServerName();
}

void IpcInterface::start()
{
    if (m_binaryPath.isEmpty()) {
        qCCritical(lcIpcInterface) << "Path to headless client executable is not set - aborting";
        return;
    }

    const QFileInfo execInfo(m_binaryPath);
    if (!execInfo.exists()) {
        qCCritical(lcIpcInterface) << "Path" << m_binaryPath << "does not exist - aborting";
        return;
    }
    if (!execInfo.isExecutable()) {
        qCCritical(lcIpcInterface) << "Path" << m_binaryPath << "is not executable - aborting";
        return;
    }

    connect(&m_process, &QProcess::readyReadStandardError, this, [this]() {
        qCritical() << "Error in subprocess:" << m_process.readAllStandardError();
        qFatal("Exiting because of subprocess error");
    });

    m_process.setProgram(m_binaryPath);
    m_process.setArguments(m_args);
    m_process.setInputChannelMode(QProcess::InputChannelMode::ManagedInputChannel);
    m_process.setReadChannel(QProcess::ProcessChannel::StandardOutput);
    m_process.start();
    m_process.waitForStarted();
}

void IpcInterface::stop()
{
    if (!m_isRunning) {
        return;
    }
    setIsRunning(false);

    m_socketRequest->close();
    m_socketRequest->deleteLater();
    m_socketRequest.clear();

    m_socketResponse->close();
    m_socketResponse->deleteLater();
    m_socketResponse.clear();

    if (m_process.state() != QProcess::NotRunning) {
        m_process.terminate();
        m_process.waitForFinished(500);
    }

    if (m_process.state() != QProcess::NotRunning) {
        m_process.kill();
        m_process.waitForFinished(100);
    }
}

quint64 IpcInterface::writeData(const QByteArray &data)
{
    // Serialize and send request
    const quint64_le byteSize = static_cast<quint64_le>(data.size());

    m_socketRequest->write(reinterpret_cast<const char *>(&byteSize), sizeof(quint64));
    m_socketRequest->write(data);

    return byteSize;
}

QString IpcInterface::fullRequestServerName() const
{
    return m_localServerRequest.fullServerName();
}

QString IpcInterface::fullResponseServerName() const
{
    return m_localServerResponse.fullServerName();
}

void IpcInterface::onReadReady()
{
    static constexpr size_t uintSize = sizeof(quint64);

    if (!m_sizeBuffer) {
        // Read size

        if (m_socketResponse->bytesAvailable() < 0
            || static_cast<quint64>(m_socketResponse->bytesAvailable()) < uintSize) {
            // Not enough bytes yet - postpone
            return;
        }

        const auto numberOfBytesRead =
                m_socketResponse->read(reinterpret_cast<char *>(&m_sizeBuffer), uintSize);
        Q_ASSERT(numberOfBytesRead == uintSize);

    } else {
        // Read payload

        if (m_socketResponse->bytesAvailable() < 0
            || static_cast<quint64>(m_socketResponse->bytesAvailable()) < m_sizeBuffer) {
            // Not enough bytes yet - postpone
            return;
        }

        enlargeBuffer(m_sizeBuffer);
        const auto numberOfBytesRead = m_socketResponse->read(m_buffer, m_sizeBuffer);
        Q_ASSERT(numberOfBytesRead > 0 && static_cast<quint64>(numberOfBytesRead) == m_sizeBuffer);

        m_sizeBuffer = 0;

        Q_EMIT blobReceived(QByteArray::fromRawData(m_buffer, numberOfBytesRead));
    }

    if (m_socketResponse->bytesAvailable()) {
        onReadReady();
    }
}

void IpcInterface::enlargeBuffer(quint64 newTotalSize)
{
    if (newTotalSize < m_currentBufferSize) {
        return;
    }

    char *temp = static_cast<char *>(realloc(m_buffer, newTotalSize * sizeof(char)));

    if (!temp) {
        qFatal("Buffer reallocation has failed");
    }

    m_buffer = temp;
    m_currentBufferSize = newTotalSize;
}

void IpcInterface::setIsRunning(bool value)
{
    if (m_isRunning != value) {
        m_isRunning = value;
        Q_EMIT isRunningChanged();
    }
}

#undef GONNECT_BUFFER_SIZE
