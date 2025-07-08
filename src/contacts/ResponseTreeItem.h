#pragma once

#include <QVariant>

class TreeItem
{
public:
    explicit TreeItem(const QVariantList &data, TreeItem *parentItem = nullptr);
    ~TreeItem();

    void appendChild(TreeItem *child);
    TreeItem *child(int row);
    int childCount() const;
    int columnCount() const;
    QVariant data(int column) const;
    int row() const;
    TreeItem *parentItem();

private:
    QList<TreeItem *> m_childItems;
    QVariantList m_itemData;
    TreeItem *m_parentItem = nullptr;
};
