#include "RandomRoomNameGenerator.h"

#include <QFile>
#include <QLoggingCategory>
#include <QRandomGenerator>
#include <QTextStream>

Q_LOGGING_CATEGORY(lcRandomRoomNameGenerator, "gonnect.app.RandomRoomNameGenerator")

// Word list is taken from
// https://github.com/jitsi/js-utils/blob/d90533e1fdd90c539d66d06c4363c1fd65c53139/random/roomNameGenerator.js
// and compiled in :/roomnames/ resources

QStringList wordListFromResource(const QString &name)
{
    QFile file(QStringLiteral(":/roomnames/%1.txt").arg(name));

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCWarning(lcRandomRoomNameGenerator)
                << "Cannot read word list" << file.fileName() << ":" << file.errorString();
        return {};
    }

    QStringList words;
    QTextStream stream(&file);

    while (!stream.atEnd()) {
        const QString word = stream.readLine().trimmed();

        if (!word.isEmpty()) {
            words.append(word);
        }
    }

    if (words.isEmpty()) {
        qCWarning(lcRandomRoomNameGenerator) << "Word list" << file.fileName() << "is empty";
    }

    return words;
}

RandomRoomNameGenerator::RandomRoomNameGenerator(QObject *parent) : QObject{ parent } { }

QString RandomRoomNameGenerator::randomJitsiRoomName() const
{
    static const QStringList adjectives = wordListFromResource(QStringLiteral("adjectives"));
    static const QStringList pluralNouns = wordListFromResource(QStringLiteral("plural-nouns"));
    static const QStringList verbs = wordListFromResource(QStringLiteral("verbs"));
    static const QStringList adverbs = wordListFromResource(QStringLiteral("adverbs"));

    return QString("%1%2%3%4")
            .arg(randomStringFrom(adjectives), randomStringFrom(pluralNouns),
                 randomStringFrom(verbs), randomStringFrom(adverbs));
}

QString RandomRoomNameGenerator::randomStringFrom(const QStringList &list) const
{
    if (list.isEmpty()) {
        return {};
    }

    return list.at(QRandomGenerator::global()->bounded(0, list.length()));
}
