#pragma once

#include <QSortFilterProxyModel>
#include <QObject>
#include <QQmlEngine>

class EmojiProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_CLASSINFO("DefaultProperty", "sourceModel")

    Q_PROPERTY(int group MEMBER m_group NOTIFY groupChanged FINAL)

public:
    explicit EmojiProxyModel(QObject *parent = nullptr);

protected:
    virtual bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    int m_group = -1;

signals:
    void groupChanged();
};
