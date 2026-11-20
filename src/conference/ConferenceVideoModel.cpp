#include "ConferenceVideoModel.h"

#include <algorithm>

ConferenceVideoModel::ConferenceVideoModel(QObject *parent) : QAbstractListModel{ parent } { }

QHash<int, QByteArray> ConferenceVideoModel::roleNames() const
{
    return {
        { static_cast<int>(Roles::Source), "source" },
        { static_cast<int>(Roles::UserId), "userId" },
        { static_cast<int>(Roles::Type), "type" },
        { static_cast<int>(Roles::IsMuted), "isMuted" },
    };
}

int ConferenceVideoModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_sources.size());
}

QVariant ConferenceVideoModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid | CheckIndexOption::ParentIsInvalid)) {
        return {};
    }

    ConferenceVideoSource *source = q_check_ptr(m_sources.at(index.row()));

    switch (static_cast<Roles>(role)) {

    case Roles::Source:
        return QVariant::fromValue(source);
    case Roles::UserId:
        return source->userId();
    case Roles::Type:
        return QVariant::fromValue(source->type());
    case Roles::IsMuted:
        return source->isMuted();
    }
    return {};
}

ConferenceVideoSource *ConferenceVideoModel::addSource(const QString &id, const QString &userId,
                                                       ConferenceVideoSource::Type type)
{
    if (auto *existing = source(id)) {
        return existing;
    }

    auto *videoSource = new ConferenceVideoSource(id, userId, type, this);

    connect(videoSource, &ConferenceVideoSource::isMutedChanged, this, [this, videoSource]() {
        const auto changed = index(static_cast<int>(m_sources.indexOf(videoSource)));
        Q_EMIT dataChanged(changed, changed, { static_cast<int>(Roles::IsMuted) });
    });

    connect(videoSource, &ConferenceVideoSource::requestedHeightChanged, this,
            &ConferenceVideoModel::requestedHeightsChanged);

    const int row = static_cast<int>(m_sources.size());
    beginInsertRows(QModelIndex(), row, row);
    m_sources.append(videoSource);
    endInsertRows();

    return videoSource;
}

void ConferenceVideoModel::removeSource(const QString &id)
{
    const auto row = indexOf(id);
    if (row < 0) {
        return;
    }

    beginRemoveRows(QModelIndex(), row, row);
    ConferenceVideoSource *removed = m_sources.takeAt(row);
    endRemoveRows();

    release(removed);
}

void ConferenceVideoModel::clear()
{
    if (m_sources.isEmpty()) {
        return;
    }

    beginResetModel();
    const QList<ConferenceVideoSource *> removed = std::exchange(m_sources, {});
    endResetModel();

    for (auto *source : removed) {
        release(source);
    }
}

ConferenceVideoSource *ConferenceVideoModel::source(const QString &id) const
{
    const auto row = indexOf(id);
    return row < 0 ? nullptr : m_sources.at(row);
}

qsizetype ConferenceVideoModel::indexOf(const QString &id) const
{
    const auto it = std::ranges::find_if(
            m_sources, [&id](const ConferenceVideoSource *source) { return source->id() == id; });
    return it == m_sources.cend() ? -1 : std::distance(m_sources.cbegin(), it);
}

void ConferenceVideoModel::release(ConferenceVideoSource *source)
{
    // Prevent errors because QML delegates might still have source reference
    source->disconnect(this);
    source->deleteLater();
}
