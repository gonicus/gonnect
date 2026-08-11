#pragma once

#include <QSortFilterProxyModel>
#include <QQmlEngine>

class ChatProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_CLASSINFO("DefaultProperty", "sourceModel")

    Q_PROPERTY(QString threadId MEMBER m_threadId NOTIFY threadIdChanged FINAL)

public:
    explicit ChatProxyModel(QObject *parent = nullptr);

protected:
    virtual bool lessThan(const QModelIndex &sourceLeft,
                          const QModelIndex &sourceRight) const override;
    virtual bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    QString m_threadId;

Q_SIGNALS:
    void threadIdChanged();
};
