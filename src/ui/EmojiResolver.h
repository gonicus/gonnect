#pragma once

#include <QObject>
#include <QHash>
#include "EmojiInfo.h"

class EmojiResolver : public QObject
{
    Q_OBJECT

public:
    static EmojiResolver &instance()
    {
        static EmojiResolver *_instance = nullptr;
        if (!_instance) {
            _instance = new EmojiResolver;
        }
        return *_instance;
    }

    const QList<const EmojiInfo *> &ordered() const { return m_ordered; }
    const QList<std::pair<QString, const EmojiInfo *>> &groupIndexes() const
    {
        return m_groupIndexes;
    }

    const EmojiInfo *emojiInfoForHexCode(const QString &hex) const;

    QString emojiForCode(const QString &code) const;
    QString replaceEmojiCodes(const QString &string) const;

private:
    explicit EmojiResolver(QObject *parent = nullptr);
    void fillData();
    void addManualMappings();

    QHash<QString, const EmojiInfo *> m_emojiInfos;
    QHash<QString, const EmojiInfo *> m_shortcodes;
    QHash<QString, QList<const EmojiInfo *>> m_tags;
    QList<const EmojiInfo *> m_ordered;
    QList<std::pair<QString, const EmojiInfo *>> m_groupIndexes;
};
