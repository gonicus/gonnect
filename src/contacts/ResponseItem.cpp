#include "ResponseItem.h"

ResponseItem::ResponseItem(QObject *parent) : QObject(parent) { }

ResponseItem::ResponseItem(const QString &category, const QString &title,
                           const QList<Item *> &items, QObject *parent)
    : QObject(parent)

{
    this->category = category;
    this->title = title;
    this->items = items;
}

ResponseItem::~ResponseItem()
{
    qDeleteAll(items);
    items.clear();
}
