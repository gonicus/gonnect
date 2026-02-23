#pragma once

#include <QAbstractListModel>
#include <QQmlEngine>

class IConferenceConnector;

class ParticipantsModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(IConferenceConnector *conferenceConnector MEMBER m_conferenceConnector NOTIFY
                       conferenceConnectorChanged FINAL)

public:
    enum class Roles { Id = Qt::UserRole + 1, DisplayName, Role };

    explicit ParticipantsModel(QObject *parent = nullptr);

    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;

private Q_SLOTS:
    void onConferenceConnectorChanged();

private:
    IConferenceConnector *m_conferenceConnector = nullptr;
    QObject *m_jistiConnectorContext = nullptr;

Q_SIGNALS:
    void conferenceConnectorChanged();
};
