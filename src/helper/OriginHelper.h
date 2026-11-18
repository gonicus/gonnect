#pragma once

#include <QUrl>

class OriginHelper
{
public:
    OriginHelper() = delete;

    /// Same scheme, host and port (default if missing)
    static bool isSameOrigin(const QUrl &first, const QUrl &second);
};
