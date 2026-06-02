#pragma once

#include <QConcatenateTablesProxyModel>
#include <QQmlEngine>

class AllChatProvidersRoomProxyModel : public QConcatenateTablesProxyModel
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit AllChatProvidersRoomProxyModel(QObject *parent = nullptr);
};
