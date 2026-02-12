#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QSortFilterProxyModel>

class TogglerProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_CLASSINFO("DefaultProperty", "sourceModel")

    Q_PROPERTY(uint displayFilter MEMBER m_displayFilter NOTIFY displayFilterChanged FINAL)

public:
    explicit TogglerProxyModel(QObject *parent = nullptr);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    uint m_displayFilter = 0;

Q_SIGNALS:
    void displayFilterChanged();
};
