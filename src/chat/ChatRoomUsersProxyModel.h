#pragma once

#include <QSortFilterProxyModel>
#include <QQmlEngine>

class ChatRoomUsersProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_CLASSINFO("DefaultProperty", "sourceModel")

    Q_PROPERTY(QString filterText MEMBER m_filterText NOTIFY filterTextChanged FINAL)
    Q_PROPERTY(QStringList excludedUserIds MEMBER m_excludedUserIds NOTIFY excludedUserIdsChanged
                       FINAL)

public:
    explicit ChatRoomUsersProxyModel(QObject *parent = nullptr);

protected:
    virtual bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    virtual bool lessThan(const QModelIndex &sourceLeft,
                          const QModelIndex &sourceRight) const override;

private:
    QString m_filterText;
    QStringList m_excludedUserIds;

Q_SIGNALS:
    void filterTextChanged();
    void excludedUserIdsChanged();
};
