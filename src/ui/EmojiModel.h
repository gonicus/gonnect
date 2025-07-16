#pragma once

#include <QAbstractListModel>
#include <QObject>
#include <QQmlEngine>

class EmojiModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit EmojiModel(QObject *parent = nullptr);

    enum class Roles { Hexcode = Qt::UserRole + 1, Emoji, Label, Group };

    virtual QHash<int, QByteArray> roleNames() const override;
    virtual int rowCount(const QModelIndex &parent) const override;
    virtual QVariant data(const QModelIndex &index, int role) const override;
};
