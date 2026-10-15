#include "PhoneNumbersModel.h"

PhoneNumbersModel::PhoneNumbersModel(QObject *parent) : QAbstractListModel{ parent }
{

    connect(this, &PhoneNumbersModel::contactChanged, this, &PhoneNumbersModel::onContactChanged);
}

QString PhoneNumbersModel::soleNumber() const
{
    if (m_contact && rowCount() == 1) {
        return m_contact->phoneNumbers().at(0).number;
    }
    return QString();
}

QHash<int, QByteArray> PhoneNumbersModel::roleNames() const
{
    return {
        { static_cast<int>(Roles::Number), "number" },
        { static_cast<int>(Roles::Type), "type" },
        { static_cast<int>(Roles::IsSipSubscribable), "isSipSubscribable" },
    };
}

int PhoneNumbersModel::rowCount(const QModelIndex &) const
{
    if (!m_contact) {
        return 0;
    }
    return m_contact->phoneNumbers().size();
}

QVariant PhoneNumbersModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || !m_contact) {
        return QVariant();
    }

    const auto &numberObj = m_contact->phoneNumbers().at(index.row());

    switch (role) {
    case static_cast<int>(Roles::Type):
        return QVariant::fromValue(numberObj.type);

    case static_cast<int>(Roles::IsSipSubscribable):
        return numberObj.isSipSubscriptable;

    case static_cast<int>(Roles::Number):
    default:
        return numberObj.number;
    }
}

void PhoneNumbersModel::onContactChanged()
{
    beginResetModel();

    if (m_contactContext) {
        delete m_contactContext;
        m_contactContext = nullptr;
    }

    if (m_contact) {
        m_contactContext = new QObject(this);
        connect(m_contact, &QObject::destroyed, m_contactContext, [this](QObject *obj) {
            if (m_contactContext && obj && obj == static_cast<QObject *>(m_contact)) {
                beginResetModel();
                delete m_contactContext;
                m_contactContext = nullptr;
                m_contact = nullptr;
                endResetModel();
                Q_EMIT countChanged();
            }
        });
    }

    endResetModel();
    Q_EMIT countChanged();
}
