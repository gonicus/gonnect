#include "SipFavoriteHelper.h"
#include "NumberStats.h"
#include "PhoneNumberUtil.h"

SipFavoriteHelper::SipFavoriteHelper(QObject *parent)
    : GenericFavoriteHelper{ NumberStats::ContactType::PhoneNumber, parent }
{
    connect(this, &SipFavoriteHelper::sipAddressChanged, this,
            [this]() { setContactString(m_sipAddress); });
}

QString SipFavoriteHelper::sanitizeContactString(const QString &value)
{
    if (PhoneNumberUtil::isNumberAnonymous(value)) {
        return "";
    }

    const QString bare = PhoneNumberUtil::bareURI(value);
    const QString user = bare.isEmpty() ? value : PhoneNumberUtil::numberFromSipUrl(bare);
    return PhoneNumberUtil::canonicalNumber(user.isEmpty() ? value : user);
}
