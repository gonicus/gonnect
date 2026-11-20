#pragma once

#include "ConferenceVideoSource.h"

#include <QAbstractListModel>
#include <qqmlregistration.h>

/// Video sources of a conference filled by conference backend
class ConferenceVideoModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

public:
    enum class Roles { Source = Qt::UserRole + 1, UserId, Type, IsMuted };

    explicit ConferenceVideoModel(QObject *parent = nullptr);

    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;

    /// Returns existing source if existent
    ConferenceVideoSource *addSource(const QString &id, const QString &userId,
                                     ConferenceVideoSource::Type type);
    void removeSource(const QString &id);
    void clear();

    ConferenceVideoSource *source(const QString &id) const;
    QList<ConferenceVideoSource *> sources() const { return m_sources; }

Q_SIGNALS:
    void requestedHeightsChanged();

private:
    qsizetype indexOf(const QString &id) const;
    void release(ConferenceVideoSource *source);

    QList<ConferenceVideoSource *> m_sources;
};
