#include "ConferenceFavoriteHelper.h"
#include "NumberStats.h"
#include "NumberStat.h"

ConferenceFavoriteHelper::ConferenceFavoriteHelper(QObject *parent)
    : GenericFavoriteHelper{ NumberStats::ContactType::JitsiMeetUrl, parent }
{
    connect(this, &ConferenceFavoriteHelper::roomNameChanged, this,
            [this]() { setContactString(m_roomName); });
}

QString ConferenceFavoriteHelper::sanitizeContactString(const QString &value)
{
    return value.trimmed();
}
