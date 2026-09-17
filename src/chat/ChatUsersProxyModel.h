#pragma once

#include <QSortFilterProxyModel>
#include <QQmlEngine>

class ChatUsersProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_CLASSINFO("DefaultProperty", "sourceModel")

    Q_PROPERTY(QStringList excludedUserIds MEMBER m_excludedUserIds NOTIFY excludedUserIdsChanged
                       FINAL)
    Q_PROPERTY(QStringList selectedUserIds MEMBER m_selectedUserIds NOTIFY selectedUserIdsChanged
                       FINAL)
    Q_PROPERTY(QString filterText MEMBER m_filterText NOTIFY filterTextChanged FINAL)

public:
    explicit ChatUsersProxyModel(QObject *parent = nullptr);

    Q_INVOKABLE void toggleSelectedState(const QString &id);

protected:
    virtual bool lessThan(const QModelIndex &sourceLeft,
                          const QModelIndex &sourceRight) const override;
    virtual bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    QStringList m_excludedUserIds;
    QStringList m_selectedUserIds;
    QString m_filterText;

Q_SIGNALS:
    void excludedUserIdsChanged();
    void selectedUserIdsChanged();
    void filterTextChanged();
};
