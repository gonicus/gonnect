#include "GenericFavoriteHelper.h"
#include "NumberStats.h"
#include "NumberStat.h"

GenericFavoriteHelper::GenericFavoriteHelper(NumberStats::ContactType contactType, QObject *parent)
    : QObject{ parent }, m_contactType{ contactType }
{
    const auto &numStats = NumberStats::instance();
    connect(&numStats, &NumberStats::modelReset, this, &GenericFavoriteHelper::updateIsFavorite);
    connect(&numStats, &NumberStats::favoriteAdded, this, [this](NumberStat *item) {
        if (item && item->contactType == m_contactType && item->phoneNumber == m_contactString) {
            updateIsFavorite();
        }
    });
    connect(&numStats, &NumberStats::favoriteRemoved, this, [this](NumberStat *item) {
        if (item && item->contactType == m_contactType && item->phoneNumber == m_contactString) {
            updateIsFavorite();
        }
    });
}

void GenericFavoriteHelper::setContactString(const QString &value)
{
    const auto newValue = sanitizeContactString(value);

    if (m_contactString != newValue) {
        m_contactString = newValue;
        updateIsFavorite();
    }
}

void GenericFavoriteHelper::toggleFavorite() const
{
    if (m_contactString.isEmpty()) {
        return;
    }

    NumberStats::instance().toggleFavorite(m_contactString, m_contactType);
}

void GenericFavoriteHelper::updateIsFavorite()
{
    bool newIsFavorite = false;

    if (!m_contactString.isEmpty()) {
        newIsFavorite = NumberStats::instance().isFavorite(m_contactString);
    }

    if (m_isFavorite != newIsFavorite) {
        m_isFavorite = newIsFavorite;
        Q_EMIT isFavoriteChanged();
    }
}
