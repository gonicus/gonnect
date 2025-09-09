#pragma once

#include <QAbstractListModel>
#include <QQmlEngine>

class IConferenceConnector;

class ParticipantsModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(IConferenceConnector *iConferenceConnector MEMBER m_iConferenceConnector NOTIFY
                       iConferenceConnectorChanged FINAL)

public:
    enum class Roles { Id = Qt::UserRole + 1, DisplayName, Role };

    explicit ParticipantsModel(QObject *parent = nullptr);

    virtual QHash<int, QByteArray> roleNames() const override;
    virtual int rowCount(const QModelIndex &parent) const override;
    virtual QVariant data(const QModelIndex &index, int role) const override;

private Q_SLOTS:
    void onIConferenceConnectorChanged();

private:
    IConferenceConnector *m_iConferenceConnector = nullptr;
    QObject *m_jistiConnectorContext = nullptr;

Q_SIGNALS:
    void iConferenceConnectorChanged();
};
