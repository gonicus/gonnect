#pragma once

#include <QSortFilterProxyModel>
#include <QQmlEngine>

class AllChatProvidersRoomSearchProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_CLASSINFO("DefaultProperty", "sourceModel")

    Q_PROPERTY(QString filterText MEMBER m_filterText NOTIFY filterTextChanged FINAL)

public:
    explicit AllChatProvidersRoomSearchProxyModel(QObject *parent = nullptr);

protected:
    virtual bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    QString m_filterText;

Q_SIGNALS:
    void filterTextChanged();
};
