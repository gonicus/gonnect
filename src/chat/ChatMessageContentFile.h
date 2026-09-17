#pragma once

#include <QObject>
#include <QQmlEngine>

class ChatMessageContentFile : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

    Q_PROPERTY(QString filePath READ filePath NOTIFY filePathChanged FINAL)
    Q_PROPERTY(QString fileName READ fileName NOTIFY fileNameChanged FINAL)
    Q_PROPERTY(qint64 fileSize READ fileSize NOTIFY filePathChanged FINAL)

public:
    explicit ChatMessageContentFile(const QString &filePath, const QString &fileName = "",
                                    QObject *parent = nullptr);

    QString filePath() const { return m_filePath; }
    void setFilePath(const QString &filePath);
    QString fileName() const { return m_fileName; }
    void setFileName(const QString &fileName);
    qint64 fileSize();

private:
    QString m_filePath;
    QString m_fileName;
    qint64 m_fileSize = 0;

Q_SIGNALS:
    void filePathChanged();
    void fileNameChanged();
};
