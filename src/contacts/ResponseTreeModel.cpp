#include "ResponseTreeModel.h"
#include "SIPCallManager.h"

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcResponseTreeModel, "gonnect.app.ResponseTreeModel")

ResponseTreeModel::ResponseTreeModel(QObject *parent) : QAbstractItemModel(parent)
{
    QList<QVariant> rootData;
    rootData << tr("Additional Information"); // INFO: Column header labels
    m_rootItem = new TreeItem(rootData);

    connect(this, &ResponseTreeModel::callIdChanged, this, &ResponseTreeModel::resetModel);
    connect(this, &ResponseTreeModel::accountIdChanged, this, &ResponseTreeModel::resetModel);
}

ResponseTreeModel::ResponseTreeModel(const QString &data, QObject *parent)
    : QAbstractItemModel(parent)
{
    Q_UNUSED(data); // INFO: Doesn't seem like we'd need initial '\n' delimited data

    QList<QVariant> rootData;
    rootData << tr("Additional Information"); // INFO: Column header labels
    m_rootItem = new TreeItem(rootData);

    connect(this, &ResponseTreeModel::callIdChanged, this, &ResponseTreeModel::resetModel);
    connect(this, &ResponseTreeModel::accountIdChanged, this, &ResponseTreeModel::resetModel);
}

ResponseTreeModel::~ResponseTreeModel()
{
    delete m_rootItem;
}

QVariant ResponseTreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole) {
        return QVariant();
    }

    const auto *item = static_cast<const TreeItem *>(index.internalPointer());
    return item->data(index.column());
}

Qt::ItemFlags ResponseTreeModel::flags(const QModelIndex &index) const
{
    return index.isValid() ? QAbstractItemModel::flags(index) : Qt::ItemFlags(Qt::NoItemFlags);
}

QVariant ResponseTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    return orientation == Qt::Horizontal && role == Qt::DisplayRole ? m_rootItem->data(section)
                                                                    : QVariant();
}

QModelIndex ResponseTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent)) {
        return QModelIndex();
    }

    TreeItem *parentItem =
            parent.isValid() ? static_cast<TreeItem *>(parent.internalPointer()) : m_rootItem;

    if (auto *childItem = parentItem->child(row)) {
        return createIndex(row, column, childItem);
    }
    return QModelIndex();
}

QModelIndex ResponseTreeModel::parent(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return QModelIndex();
    }

    auto *childItem = static_cast<TreeItem *>(index.internalPointer());
    TreeItem *parentItem = childItem->parentItem();

    return parentItem != m_rootItem ? createIndex(parentItem->row(), 0, parentItem) : QModelIndex{};
}

int ResponseTreeModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0) {
        return 0;
    }

    const TreeItem *parentItem =
            parent.isValid() ? static_cast<const TreeItem *>(parent.internalPointer()) : m_rootItem;

    return parentItem->childCount();
}

int ResponseTreeModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? static_cast<TreeItem *>(parent.internalPointer())->columnCount()
                            : m_rootItem->columnCount();
}

QHash<int, QByteArray> ResponseTreeModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[Qt::DisplayRole] = "output";

    return roles;
}

void ResponseTreeModel::setupModelData(const QStringList &lines, TreeItem *parent)
{
    for (const QString &line : lines) {
        QList<QVariant> data;
        data << line;

        parent->appendChild(new TreeItem(data, parent));
    }
}

void ResponseTreeModel::loadModelData()
{
    const QList<ResponseItem *> response = m_call->metadata();
    if (response.empty()) {
        qCDebug(lcResponseTreeModel, "No metadata found for the current call");
        return;
    }

    for (const auto entry : response) {
        if (entry) {
            // INFO: QVariantList with only one item because we only use 1 column
            TreeItem *ent = new TreeItem({ entry->title }, m_rootItem);
            m_rootItem->appendChild(ent);

            const QList<ResponseItem::Item *> items = entry->items;
            for (const auto item : items) {
                TreeItem *iTitle = new TreeItem({ item->title }, ent);
                ent->appendChild(iTitle);

                if (item->description != "") {
                    TreeItem *description = new TreeItem({ item->description }, iTitle);
                    iTitle->appendChild(description);
                }

                if (item->created) {
                    TreeItem *created = new TreeItem(
                            { QString("Created %1").arg(item->created->date) }, iTitle);
                    iTitle->appendChild(created);

                    TreeItem *cName = new TreeItem({ item->created->name }, created);
                    TreeItem *cMail = new TreeItem({ item->created->mail }, created);
                    TreeItem *cSipUri = new TreeItem({ item->created->sipUri }, created);
                    created->appendChild(cName);
                    created->appendChild(cMail);
                    created->appendChild(cSipUri);
                }

                if (item->latestActivity) {
                    TreeItem *latestActivity = new TreeItem(
                            { QString("Latest Activity %1").arg(item->latestActivity->date) },
                            iTitle);
                    iTitle->appendChild(latestActivity);

                    TreeItem *lName = new TreeItem({ item->latestActivity->name }, latestActivity);
                    TreeItem *lMail = new TreeItem({ item->latestActivity->mail }, latestActivity);
                    TreeItem *lSipUri =
                            new TreeItem({ item->latestActivity->sipUri }, latestActivity);
                    latestActivity->appendChild(lName);
                    latestActivity->appendChild(lMail);
                    latestActivity->appendChild(lSipUri);
                }
            }
        }
    }

    qCDebug(lcResponseTreeModel, "Processed metadata for the current call");
}

void ResponseTreeModel::checkMetadata()
{
    if (!m_call->metadata().empty()) {
        beginResetModel();

        delete m_rootItem;

        QList<QVariant> rootData;
        rootData << tr("Additional Information");
        m_rootItem = new TreeItem(rootData);

        loadModelData();

        endResetModel();
    }
}

void ResponseTreeModel::resetModel()
{
    beginResetModel();

    delete m_rootItem;

    QList<QVariant> rootData;
    rootData << tr("Additional Information");
    m_rootItem = new TreeItem(rootData);

    endResetModel();

    m_call = SIPCallManager::instance().findCall(m_accountId, m_callId);
    if (m_call) {
        QObject::connect(
                m_call, &SIPCall::metadataChanged, this, [this]() { checkMetadata(); },
                Qt::ConnectionType::SingleShotConnection);
    }
}
