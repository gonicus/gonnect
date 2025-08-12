#include "EmojiInfo.h"

EmojiInfo::EmojiInfo(QObject *parent) : QObject(parent) { }

EmojiInfo::EmojiInfo(const QString &hex, const QString &emoji, const QString &label, quint8 group,
                     const QStringList &tags, QObject *parent)
    : QObject(parent)

{
    this->hex = hex;
    this->emoji = emoji;
    this->label = label;
    this->group = group;
    this->tags = tags;
}

QDataStream &operator<<(QDataStream &out, const EmojiInfo &emojiInfo)
{
    out << emojiInfo.hex << emojiInfo.emoji << emojiInfo.label << emojiInfo.group << emojiInfo.tags;
    return out;
}
QDataStream &operator>>(QDataStream &in, EmojiInfo &emojiInfo)
{
    in >> emojiInfo.hex >> emojiInfo.emoji >> emojiInfo.label >> emojiInfo.group >> emojiInfo.tags;
    return in;
}
