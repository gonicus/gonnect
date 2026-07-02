#pragma once
#include <QString>
#include <qobjectdefs.h>

struct SIPCallRoutingHop
{
    // SIP URI of hop
    QString uri;

    // Reason for HOP, SIP code or -1 if unknown
    int reason = -1;

    // Translated human readable reason
    QString reasonText;

    // History info index or empty
    QString index;

    // Source for this hop: true for diversion, false for history-info
    bool diversion = false;
};
