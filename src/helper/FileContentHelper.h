#pragma once

#include <QString>

class FileContentHelper
{
public:
    enum class FileType { Image, Audio, Video, Other };

    static FileContentHelper::FileType fileType(const QString &filePath);

private:
    FileContentHelper();
};
