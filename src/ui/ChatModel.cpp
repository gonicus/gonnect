#include "ChatModel.h"
#include "EmojiResolver.h"
#include "ConferenceChatMessage.h"
#include <QRegularExpression>

ChatModel::ChatModel(QObject *parent) : QAbstractListModel{ parent }
{
    connect(this, &ChatModel::iConferenceConnectorChanged, this,
            &ChatModel::onIConferenceConnectorChanged);
}

QHash<int, QByteArray> ChatModel::roleNames() const
{
    return {
        { static_cast<int>(Roles::FromId), "fromId" },
        { static_cast<int>(Roles::NickName), "nickName" },
        { static_cast<int>(Roles::Message), "message" },
        { static_cast<int>(Roles::Timestamp), "timestamp" },
        { static_cast<int>(Roles::IsPrivateMessage), "isPrivateMessage" },
        { static_cast<int>(Roles::IsOwnMessage), "isOwnMessage" },
        { static_cast<int>(Roles::IsSystemMessage), "isSystemMessage" },
    };
}

int ChatModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_iConferenceConnector ? m_iConferenceConnector->messages().size() : 0;
}

QVariant ChatModel::data(const QModelIndex &index, int role) const
{
    if (!m_iConferenceConnector) {
        return QVariant();
    }

    const auto item = m_iConferenceConnector->messages().at(index.row());

    switch (role) {
    case static_cast<int>(Roles::FromId):
        return item->fromId();
    case static_cast<int>(Roles::NickName):
        return item->nickName();
    case static_cast<int>(Roles::Message):
        return addLinkTags(EmojiResolver::instance().replaceEmojiCodes(item->message()));
    case static_cast<int>(Roles::Timestamp):
        return item->timestamp();
    case static_cast<int>(Roles::IsPrivateMessage):
        return item->isPrivateMessage();
    case static_cast<int>(Roles::IsOwnMessage):
        return item->fromId() == m_iConferenceConnector->ownId();
    case static_cast<int>(Roles::IsSystemMessage):
        return item->isSystemMessage();
    default:
        return QVariant();
    }
}

void ChatModel::onIConferenceConnectorChanged()
{
    beginResetModel();

    if (m_iConferenceConnectorContext) {
        m_iConferenceConnectorContext->deleteLater();
        m_iConferenceConnectorContext = nullptr;
    }

    if (m_iConferenceConnector) {
        m_iConferenceConnectorContext = new QObject(this);
        connect(m_iConferenceConnector, &IConferenceConnector::chatMessageAdded,
                m_iConferenceConnectorContext, [this](qsizetype index, ConferenceChatMessage *) {
                    beginInsertRows(QModelIndex(), index, index);
                    endInsertRows();
                    updateRealMessagesCount();
                });
        connect(m_iConferenceConnector, &IConferenceConnector::chatMessagesReset,
                m_iConferenceConnectorContext, [this]() {
                    beginResetModel();
                    endResetModel();
                    updateRealMessagesCount();
                });
    }

    endResetModel();
    updateRealMessagesCount();
}

void ChatModel::updateRealMessagesCount()
{
    uint count = 0;

    if (m_iConferenceConnector) {
        const auto &messages = m_iConferenceConnector->messages();
        for (const auto &message : messages) {
            if (!message->isSystemMessage()) {
                ++count;
            }
        }
    }

    if (m_realMessagesCount != count) {
        m_realMessagesCount = count;
        Q_EMIT realMessagesCountChanged();
    }
}

QString ChatModel::addLinkTags(const QString &orig) const
{
    static const QRegularExpression re("^\\S+\\.\\S{2,}$",
                                       QRegularExpression::CaseInsensitiveOption);

    auto split = orig.split(' ');

    QMutableListIterator it(split);
    while (it.hasNext()) {
        auto s = it.next();

        const auto match = re.match(s);
        if (match.hasMatch()) {
            it.setValue(QString("<a href=\"%1\">%1</a>").arg(s));
        }
    }

    return split.join(' ');
}
