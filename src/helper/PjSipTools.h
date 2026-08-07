#pragma once
#include <QString>
#include <pjsua-lib/pjsua.h>

class PjSipTools
{
public:
    static QString pjStringToQstring(const pj_str_t &str)
    {
        return QString::fromUtf8(str.ptr, static_cast<int>(str.slen));
    }
};
