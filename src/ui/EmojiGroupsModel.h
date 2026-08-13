#pragma once

#include <QAbstractListModel>
#include <QObject>
#include <QQmlEngine>

class EmojiGroupsModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit EmojiGroupsModel(QObject *parent = nullptr);

    enum class Roles { GroupIndex = Qt::UserRole + 1, Emoji };

    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
};
