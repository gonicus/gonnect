#include "ChatMessageContentFile.h"
#include <QFileInfo>

ChatMessageContentFile::ChatMessageContentFile(const QString &filePath, const QString &fileName,
                                               QObject *parent)
    : QObject{ parent }, m_filePath{ filePath }, m_fileName{ fileName }
{
}

void ChatMessageContentFile::setFilePath(const QString &filePath)
{
    if (m_filePath != filePath) {
        m_filePath = filePath;
        m_fileSize = 0;
        Q_EMIT filePathChanged();
    }
}

void ChatMessageContentFile::setFileName(const QString &fileName)
{
    if (m_fileName != fileName) {
        m_fileName = fileName;
        Q_EMIT fileNameChanged();
    }
}

qint64 ChatMessageContentFile::fileSize()
{
    if (m_fileSize != 0) {
        return m_fileSize;
    }
    if (m_filePath.isEmpty()) {
        return 0;
    }

    static const QString fileProtocol("file://");

    auto path = m_filePath;
    if (path.startsWith(fileProtocol)) {
        path.remove(0, fileProtocol.length());
    }

    QFileInfo info(path);
    m_fileSize = info.size();
    return m_fileSize;
}
