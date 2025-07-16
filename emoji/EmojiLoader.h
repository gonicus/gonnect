#pragma once

#include <QObject>
#include <QJsonDocument>

class EmojiLoader : public QObject
{
    Q_OBJECT

public:
    explicit EmojiLoader(QObject *parent = nullptr);

    void loadEmojis(const QString &sourceDir, const QString &targetDir);

private:
    void assrt(bool cond, QString err);
    void processData();
    void readJsonFile(const QString &path, QJsonDocument &doc);

signals:
    void finished();
    void error(QString err);
};
