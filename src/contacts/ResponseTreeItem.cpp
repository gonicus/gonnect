#include "ResponseTreeItem.h"

TreeItem::TreeItem(const QVariantList &data, TreeItem *parentItem)
    : m_itemData(std::move(data)), m_parentItem(parentItem)
{
}

TreeItem::~TreeItem()
{
    qDeleteAll(m_childItems);
    m_childItems.clear();
}

void TreeItem::appendChild(TreeItem *child)
{
    m_childItems.append(std::move(child));
}

TreeItem *TreeItem::child(int row)
{
    return row >= 0 && row < childCount() ? m_childItems.value(row) : nullptr;
}

int TreeItem::childCount() const
{
    return static_cast<int>(m_childItems.size());
}

int TreeItem::columnCount() const
{
    return static_cast<int>(m_itemData.count());
}

QVariant TreeItem::data(int column) const
{
    return m_itemData.value(column);
}

int TreeItem::row() const
{
    if (m_parentItem) {
        return m_parentItem->m_childItems.indexOf(const_cast<TreeItem *>(this));
    }

    return 0;
}

TreeItem *TreeItem::parentItem()
{
    return m_parentItem;
}
