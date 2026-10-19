#pragma once

#include <QAbstractListModel>
#include <QQmlEngine>

#include "IConferenceConnector.h"

class TranscriptionChunksModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(IConferenceConnector *conferenceConnector MEMBER m_connector NOTIFY
                       conferenceConnectorChanged FINAL)

public:
    enum class Roles {
        MessageId = Qt::UserRole + 1,
        ParticipantId,
        ParticipantName,
        ParticipantAvatarUrl,
        Language,
        FinalText,
        StableText,
        UnstableText,
        ReceivedAt,
    };

    explicit TranscriptionChunksModel(QObject *parent = nullptr);

    virtual QHash<int, QByteArray> roleNames() const override;
    virtual int rowCount(const QModelIndex &parent) const override;
    virtual QVariant data(const QModelIndex &index, int role) const override;

private Q_SLOTS:
    void onConnectorChanged();

private:
    IConferenceConnector *m_connector = nullptr;
    QObject *m_connectorContext = nullptr;

Q_SIGNALS:
    void conferenceConnectorChanged();
};
