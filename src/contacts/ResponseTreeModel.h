#pragma once
#include "ResponseTreeItem.h"
#include "SIPCall.h"

#include <QAbstractItemModel>
#include <QQmlEngine>
#include <QPointer>

class ResponseTreeModel : public QAbstractItemModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(int callId MEMBER m_callId NOTIFY callIdChanged FINAL)
    Q_PROPERTY(QString accountId MEMBER m_accountId NOTIFY accountIdChanged FINAL)

public:
    Q_DISABLE_COPY_MOVE(ResponseTreeModel)

    explicit ResponseTreeModel(QObject *parent = nullptr);
    explicit ResponseTreeModel(const QString &data, QObject *parent = nullptr);
    ~ResponseTreeModel() override;

    QVariant data(const QModelIndex &index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = {}) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = {}) const override;
    int columnCount(const QModelIndex &parent = {}) const override;
    QHash<int, QByteArray> roleNames() const override;

    void checkMetadata();
    void loadModelData();

private Q_SLOTS:
    void resetModel();

private:
    static void setupModelData(const QStringList &lines, TreeItem *parent);

    TreeItem *m_rootItem = nullptr;

    int m_callId;
    QString m_accountId;
    QPointer<SIPCall> m_call;

Q_SIGNALS:
    void callIdChanged();
    void accountIdChanged();
};
