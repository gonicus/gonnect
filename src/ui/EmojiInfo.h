#pragma once
#include <QObject>

class EmojiInfo : public QObject
{
    Q_OBJECT
public:
    EmojiInfo(QObject *parent = nullptr);
    EmojiInfo(const QString &hex, const QString &emoji, const QString &label, quint8 group,
              const QStringList &tags, QObject *parent);

    QString hex;
    QString emoji;
    QString label;
    quint8 group = 0;
    QStringList tags;
};

QDataStream &operator<<(QDataStream &out, const EmojiInfo &emojiInfo);
QDataStream &operator>>(QDataStream &in, EmojiInfo &emojiInfo);
