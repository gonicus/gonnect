#include "EmojiModel.h"
#include "EmojiResolver.h"

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcEmojiModel, "gonnect.app.EmojiModel")

EmojiModel::EmojiModel(QObject *parent) : QAbstractListModel{ parent } { }

QHash<int, QByteArray> EmojiModel::roleNames() const
{
    return {
        { static_cast<int>(Roles::Hexcode), "hexcode" },
        { static_cast<int>(Roles::Emoji), "emoji" },
        { static_cast<int>(Roles::Label), "label" },
        { static_cast<int>(Roles::Group), "group" },
    };
}

int EmojiModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return EmojiResolver::instance().ordered().size();
}

QVariant EmojiModel::data(const QModelIndex &index, int role) const
{
    const auto emojiInfo = EmojiResolver::instance().ordered().at(index.row());

    if (!emojiInfo) {
        qCWarning(lcEmojiModel) << "Index" << index.row()
                                << "yields a nullptr and no EmojiInfo object";
        return QVariant();
    }

    switch (role) {
    case static_cast<int>(Roles::Hexcode):
        return emojiInfo->hex;
    case static_cast<int>(Roles::Emoji):
        return emojiInfo->emoji;
    case static_cast<int>(Roles::Group):
        return emojiInfo->group;
    case static_cast<int>(Roles::Label):
    default:
        return emojiInfo->label;
    }
}
