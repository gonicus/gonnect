#pragma once

#include <QSortFilterProxyModel>
#include <QQmlEngine>

class IChatProvider;
class IChatRoom;

class ConferenceLinkedRoomsProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
    Q_CLASSINFO("DefaultProperty", "sourceModel")
    QML_ELEMENT

    Q_PROPERTY(QString conferenceUrl READ conferenceUrl WRITE setConferenceUrl NOTIFY
                       conferenceUrlChanged FINAL)

public:
    explicit ConferenceLinkedRoomsProxyModel(QObject *parent = nullptr);

    QString conferenceUrl() const { return m_conferenceUrl; }
    void setConferenceUrl(const QString &conferenceUrl);

    Q_INVOKABLE QString nameAt(int row) const;
    Q_INVOKABLE IChatRoom *chatRoomAt(int row) const;
    Q_INVOKABLE IChatProvider *chatProviderAt(int row) const;

protected:
    virtual bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    QString m_conferenceUrl;

Q_SIGNALS:
    void conferenceUrlChanged();
};
