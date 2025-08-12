#pragma once

#include <QSortFilterProxyModel>
#include <QQmlEngine>

class HistoryContactSearchProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_CLASSINFO("DefaultProperty", "sourceModel")

    Q_PROPERTY(bool showJitsi MEMBER m_showJitsi NOTIFY showJitsiChanged FINAL)

public:
    explicit HistoryContactSearchProxyModel(QObject *parent = nullptr);

protected:
    virtual bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    bool m_showJitsi = true;

signals:
    void showJitsiChanged();
};
