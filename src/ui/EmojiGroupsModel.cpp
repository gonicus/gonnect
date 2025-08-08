#include "EmojiGroupsModel.h"
#include "EmojiResolver.h"

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcEmojiGroupsModel, "gonnect.app.EmojiGroupsModel")

EmojiGroupsModel::EmojiGroupsModel(QObject *parent) : QAbstractListModel{ parent } { }

QHash<int, QByteArray> EmojiGroupsModel::roleNames() const
{
    return {
        { static_cast<int>(Roles::GroupIndex), "groupIndex" },
        { static_cast<int>(Roles::Emoji), "emoji" },
    };
}

int EmojiGroupsModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return EmojiResolver::instance().groupIndexes().size();
}

QVariant EmojiGroupsModel::data(const QModelIndex &index, int role) const
{

    const auto &pairItem = EmojiResolver::instance().groupIndexes().at(index.row());

    if (!pairItem.second) {
        qCWarning(lcEmojiGroupsModel) << "No emoji info item for group" << pairItem.first;
        return QVariant();
    }

    switch (role) {
    case static_cast<int>(Roles::GroupIndex):
        return pairItem.first;
    case static_cast<int>(Roles::Emoji):
        return pairItem.second->emoji;
    default:
        return pairItem.second->label;
    }
}
