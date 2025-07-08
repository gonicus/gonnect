#pragma once

#include <QSortFilterProxyModel>
#include <QQmlEngine>

class SearchListProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_CLASSINFO("DefaultProperty", "sourceModel")
    Q_PROPERTY(QString sourceDisplayName MEMBER m_sourceDisplayName NOTIFY sourceDisplayNameChanged
                       FINAL)

public:
    explicit SearchListProxyModel(QObject *parent = nullptr);

protected:
    virtual bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    QString m_sourceDisplayName;

signals:
    void sourceDisplayNameChanged();
};
