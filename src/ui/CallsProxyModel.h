#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QSortFilterProxyModel>

class CallsProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_CLASSINFO("DefaultProperty", "sourceModel")

    Q_PROPERTY(bool onlyEstablishedCalls MEMBER m_onlyEstablishedCalls NOTIFY
                       onlyEstablishedCallsChanged FINAL)
    Q_PROPERTY(bool hideIncomingSecondaryCallOnBusy MEMBER m_hideIncomingSecondaryCallOnBusy NOTIFY
                       hideIncomingSecondaryCallOnBusyChanged FINAL)

public:
    explicit CallsProxyModel(QObject *parent = nullptr);

protected:
    virtual bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    bool m_onlyEstablishedCalls = false;
    bool m_hideIncomingSecondaryCallOnBusy = false;

signals:
    void onlyEstablishedCallsChanged();
    void hideIncomingSecondaryCallOnBusyChanged();
};
