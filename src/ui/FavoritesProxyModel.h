#pragma once

#include <QSortFilterProxyModel>
#include <QObject>
#include <QQmlEngine>

class FavoritesProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_CLASSINFO("DefaultProperty", "sourceModel")

    Q_PROPERTY(bool showJitsi MEMBER m_showJitsi NOTIFY showJitsiChanged FINAL)

public:
    explicit FavoritesProxyModel(QObject *parent = nullptr);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    bool m_showJitsi = true;

Q_SIGNALS:
    void showJitsiChanged();
};
