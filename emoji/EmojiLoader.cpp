#include "EmojiLoader.h"
#include "../src/ui/EmojiInfo.h"

#include <QMetaEnum>
#include <QJsonParseError>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QFile>
#include <QDirIterator>

EmojiLoader::EmojiLoader(QObject *parent) : QObject{ parent } { }

void EmojiLoader::loadEmojis(const QString &sourceDirPath, const QString &targetDirPath)
{
    QDir sourceDir(sourceDirPath);
    assrt(sourceDir.exists(), "Source path does not exist");
    assrt(sourceDir.isReadable(), "Source path is not readable");

    QDir targetDir(targetDirPath);
    targetDir.makeAbsolute();
    targetDir.mkpath(targetDir.absolutePath());

    QJsonDocument dataDoc;
    QJsonDocument shortCodeDoc;

    readJsonFile(sourceDir.absoluteFilePath("data.json"), dataDoc);
    readJsonFile(sourceDir.absoluteFilePath("emojibase.json"), shortCodeDoc);

    assrt(dataDoc.isArray(), "data json must be an array");

    QList<std::pair<quint32, QString>> groups;
    const auto maxGroup = std::numeric_limits<quint8>().max();
    const quint32 maxBitVal = (1 << 8) - 1;

    // Emoji infos
    EmojiInfo info;
    QFile file(targetDir.absoluteFilePath("emojis.dat"));
    assrt(file.open(QIODevice::WriteOnly), "Cannot open emojis.dat");
    QDataStream out(&file);

    const auto arr = dataDoc.array();
    for (const auto el : arr) {
        assrt(el.isObject(), "elements on main level objects");
        const auto obj = el.toObject();
        assrt(obj.contains("hexcode"), "main element must contain key 'hexcode'");
        assrt(obj.contains("emoji"), "main element must contain key 'emoji'");
        assrt(obj.contains("label"), "main element must contain key 'label'");

        info.hex = obj.value("hexcode").toString();
        info.emoji = obj.value("emoji").toString();
        info.label = obj.value("label").toString();
        info.group = obj.value("group").toInt(maxGroup);
        info.tags = {};

        if (obj.contains("tags")) {
            const auto tags = obj.value("tags").toArray();
            for (const auto tag : tags) {
                const auto tagStr = tag.toString();
                if (!tagStr.isEmpty()) {
                    info.tags.append(tagStr);
                }
            }
        }

        out << info;

        // Group sort key
        const quint32 group = obj.value("group").toInt(maxBitVal);
        const quint32 subGroup = obj.value("subgroup").toInt(maxBitVal);
        const quint32 order = obj.value("order").toInt(maxBitVal);

        groups.append(std::make_pair<quint32, QString>(group << 16 | subGroup << 8 | order,
                                                       QString(info.hex)));
    }

    file.close();

    std::sort(groups.begin(), groups.end(),
              [](const auto &left, const auto &right) { return left.first < right.first; });

    // Shortcodes
    QFile shortCodeFile(targetDir.filePath("shortcodes.dat"));

    assrt(shortCodeFile.open(QIODevice::WriteOnly), "Cannot open shortcodes.dat");
    QDataStream shortCodeOut(&shortCodeFile);

    const auto shortCodes = shortCodeDoc.object();

    for (auto it = shortCodes.constBegin(); it != shortCodes.constEnd(); ++it) {
        const auto code = it.value().toString();
        if (!code.isEmpty()) {
            shortCodeOut << code << it.key();
        }
    }
    shortCodeFile.close();

    // Ordered and group index
    QFile orderedFile(targetDir.filePath("ordered.dat"));
    assrt(orderedFile.open(QIODevice::WriteOnly), "Cannot open ordered.dat");
    QDataStream orderOut(&orderedFile);

    QFile groupFile(targetDir.filePath("groups.dat"));
    assrt(groupFile.open(QIODevice::WriteOnly), "Cannot open groups.dat");
    QDataStream groupOut(&groupFile);

    quint8 lastGroup = maxBitVal - 1;
    for (const auto &pair : std::as_const(groups)) {
        orderOut << pair.second;

        const quint8 group = pair.first >> 16;
        if (group != lastGroup) {
            lastGroup = group;
            groupOut << QString::number(group) << pair.second;
        }
    }
    orderedFile.close();
    groupFile.close();

    qInfo() << arr.size() << "emojis processed";

    emit finished();
}

void EmojiLoader::readJsonFile(const QString &path, QJsonDocument &doc)
{
    QFile file(path);
    assrt(file.exists(), path + " does not exist");
    assrt(file.open(QIODevice::ReadOnly | QIODevice::Text), "Unable to open " + path);

    QJsonParseError jsonErr;
    doc = QJsonDocument::fromJson(file.readAll(), &jsonErr);
    file.close();

    assrt(!jsonErr.error, jsonErr.errorString());
}

void EmojiLoader::assrt(bool cond, QString err)
{
    if (!cond) {
        qFatal("%s", err.toStdString().c_str());
    }
}
