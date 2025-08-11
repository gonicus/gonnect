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

    virtual QHash<int, QByteArray> roleNames() const override;
    virtual int rowCount(const QModelIndex &parent) const override;
    virtual QVariant data(const QModelIndex &index, int role) const override;
};
