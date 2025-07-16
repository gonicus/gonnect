#include <QCoreApplication>
#include <QDebug>
#include "EmojiLoader.h"

int main(int argc, char *argv[])
{
    if (argc != 3) {
        qCritical() << "Insufficient arguments. Usage: update-emojis source_dir target_dir";
        return 1;
    }

    EmojiLoader loader;
    loader.loadEmojis((QString(argv[1])), (QString(argv[2])));

    return 0;
}
