#pragma once

#include <QAbstractListModel>
#include <QQmlEngine>

#include "ChatMessageSearchIndexer.h"

class ChatMessageSearchModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

public:
    enum class Roles { MessageUid = Qt::UserRole + 1, Rank };

    explicit ChatMessageSearchModel(QObject *parent = nullptr);

    virtual QHash<int, QByteArray> roleNames() const override;
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    virtual QVariant data(const QModelIndex &index, int role) const override;

    void addResults(const QList<ChatMessageSearchIndexer::SearchResult> &results);

    void reset();

private:
    QList<ChatMessageSearchIndexer::SearchResult> m_results;
};
