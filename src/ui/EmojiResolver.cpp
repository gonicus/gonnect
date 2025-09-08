#include "EmojiResolver.h"

#include <QFile>
#include <QHash>
#include <QRegularExpression>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcEmojiResolver, "gonnect.app.EmojiResolver")

EmojiResolver::EmojiResolver(QObject *parent) : QObject{ parent }
{
    fillData();
    addManualMappings();
}

const EmojiInfo *EmojiResolver::emojiInfoForHexCode(const QString &hex) const
{
    return m_emojiInfos.value(hex, nullptr);
}

QString EmojiResolver::emojiForCode(const QString &code) const
{
    if (const auto emojiInfo = m_shortcodes.value(code, nullptr)) {
        return emojiInfo->emoji;
    }
    return "";
}

QString EmojiResolver::replaceEmojiCodes(const QString &string) const
{
    static const QRegularExpression codeRe(":[a-zA-Z0-9_+-]{2,}?:");

    auto result = string;
    auto it = codeRe.globalMatch(string);

    while (it.hasNext()) {
        const auto match = it.next();
        const auto code = match.captured(0);

        if (code.size() > 2) {
            const auto codeStripped = code.sliced(1, code.size() - 2);
            if (const auto emojiInfo = m_shortcodes.value(codeStripped, nullptr)) {
                result.replace(code, emojiInfo->emoji);
            }
        }
    }

    return result;
}

void EmojiResolver::fillData()
{
    QFile file(":/emojis/emojis.dat");
    QFile shortCodesFile(":/emojis/shortcodes.dat");
    QFile orderedFile(":/emojis/ordered.dat");
    QFile groupsFile(":/emojis/groups.dat");

    // Emojis and tags
    if (!file.open(QIODevice::ReadOnly)) {
        qFatal("failed to open emojis.dat");
    }

    QDataStream in(&file);

    while (!in.atEnd()) {
        auto emojiInfo = new EmojiInfo(this);
        in >> *emojiInfo;
        m_emojiInfos.insert(emojiInfo->hex, emojiInfo);

        for (const QString &tag : std::as_const(emojiInfo->tags)) {
            m_tags[tag].append(emojiInfo);
        }
    }
    file.close();

    // Shortcodes
    if (!shortCodesFile.open(QIODevice::ReadOnly)) {
        qFatal("failed to open shortcodes.dat");
    }

    QDataStream shortCodesIn(&shortCodesFile);

    while (!shortCodesIn.atEnd()) {
        QString key;
        QString emojiId;
        shortCodesIn >> key >> emojiId;
        m_shortcodes.insert(key, m_emojiInfos.value(emojiId, nullptr));
    }
    shortCodesFile.close();

    // Order
    if (!orderedFile.open(QIODevice::ReadOnly)) {
        qFatal("failed to open ordered.dat");
    }
    QDataStream orderedIn(&orderedFile);

    while (!orderedIn.atEnd()) {
        QString key;
        orderedIn >> key;
        m_ordered.append(m_emojiInfos.value(key, nullptr));
    }
    orderedFile.close();

    // Group indexes
    if (!groupsFile.open(QIODevice::ReadOnly)) {
        qFatal("failed to open groups.dat");
    }
    QDataStream groupsIn(&groupsFile);

    while (!groupsIn.atEnd()) {
        QString idx;
        QString hex;
        groupsIn >> idx >> hex;
        m_groupIndexes.append(std::make_pair(idx, m_emojiInfos.value(hex, nullptr)));
    }
    groupsFile.close();

    qCInfo(lcEmojiResolver).noquote().nospace()
            << "Loaded " << m_emojiInfos.size() << " emojis, " << m_shortcodes.size()
            << " short codes, " << m_tags.size() << " tags and " << m_groupIndexes.size()
            << " groups";
}

void EmojiResolver::addManualMappings()
{
    m_shortcodes.insert("angry", m_emojiInfos.value("1F620"));
    m_shortcodes.insert("scream", m_emojiInfos.value("1F631"));
    m_shortcodes.insert("wave", m_emojiInfos.value("1F44B"));
    m_shortcodes.insert("blush", m_emojiInfos.value("1F60A"));
    m_shortcodes.insert("slightly_smiling_face", m_emojiInfos.value("1F642"));
    m_shortcodes.insert("heart", m_emojiInfos.value("2764"));
    m_shortcodes.insert("innocent", m_emojiInfos.value("1F607"));
    m_shortcodes.insert("clap", m_emojiInfos.value("1F44F"));
    m_shortcodes.insert("+1", m_emojiInfos.value("1F44D"));
    m_shortcodes.insert("-1", m_emojiInfos.value("1F44E"));
}
