#pragma once

#include "ResponseItem.h"

#include <QNetworkReply>
#include <QObject>
#include <QJsonDocument>
#include <QJsonObject>

class ResponseLoader : public QObject
{
    Q_OBJECT

public:
    explicit ResponseLoader(const QString &sourceData, QObject *parent = nullptr);

    QList<ResponseItem *> loadResponse();

private slots:

private:
    void assrt(bool cond, QString err);

    QString m_sourceData;
    QJsonDocument m_dataDoc;

signals:
    void finished();
    void error(QString err);
};
