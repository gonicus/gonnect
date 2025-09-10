#pragma once

#include <QSortFilterProxyModel>
#include <QQmlEngine>

class SimpleSortProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_CLASSINFO("DefaultProperty", "sourceModel")

    Q_PROPERTY(QString sortRoleName MEMBER m_sortRoleName NOTIFY sortRoleNameChanged FINAL)

public:
    explicit SimpleSortProxyModel(QObject *parent = nullptr);

private Q_SLOTS:
    void updateSorting();

private:
    QString m_sortRoleName;

Q_SIGNALS:
    void sortRoleNameChanged();
};
