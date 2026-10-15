#pragma once

#include <QObject>
#include <QQmlEngine>

class ChatMessageContentImage : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

    Q_PROPERTY(QUrl imagePath READ imagePath NOTIFY imagePathChanged FINAL)

public:
    explicit ChatMessageContentImage(const QUrl &imagePath, QObject *parent = nullptr);

    QUrl imagePath() const { return m_imagePath; }
    void setImagePath(const QUrl &imagePath);

private:
    QUrl m_imagePath;

Q_SIGNALS:
    void imagePathChanged();
};
