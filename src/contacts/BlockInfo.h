#pragma once

#include <QDataStream>
#include <QtGlobal>

// SIP code to answer with if contact is blacklisted
// Default is 603 (Declined)
#define GONNECT_DEFAULT_BLOCK_SIP_CODE 603

struct BlockInfo
{
    bool isBlocking = false;
    quint16 responseCode = GONNECT_DEFAULT_BLOCK_SIP_CODE;
};

QDataStream &operator<<(QDataStream &out, const BlockInfo &blockInfo);
QDataStream &operator>>(QDataStream &in, BlockInfo &blockInfo);
