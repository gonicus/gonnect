#include "BlockInfo.h"

QDataStream &operator<<(QDataStream &out, const BlockInfo &blockInfo)
{
    out << blockInfo.isBlocking << blockInfo.responseCode;
    return out;
}
QDataStream &operator>>(QDataStream &in, BlockInfo &blockInfo)
{
    in >> blockInfo.isBlocking >> blockInfo.responseCode;
    return in;
}
