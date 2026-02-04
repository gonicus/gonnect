#pragma once

#include "Contact.h"

#include <QObject>
#include <QHash>
#include <QMutex>

class AddressBook : public QObject
{
    Q_OBJECT

public:
    static AddressBook &instance()
    {
        static AddressBook *_instance = nullptr;
        if (!_instance) {
            _instance = new AddressBook;
        }
        return *_instance;
    };

    Contact *addContact(const QString &dn, const QString &sourceUid,
                        const Contact::ContactSourceInfo &contactSourceInfo, const QString &name,
                        const QString &company, const QString &mail, const QDateTime &lastModified,
                        const QList<Contact::PhoneNumber> &phoneNumbers, bool block);

    void addContact(Contact *contact);

    Contact *modifyContact(const QString &dn, const QString &sourceUid, const QString &name,
                           const QString &company, const QString &mail,
                           const QDateTime &lastModified,
                           const QList<Contact::PhoneNumber> &phoneNumbers);

    void removeContact(const QString &sourceUid);

    QHash<QString, Contact *> contacts() const;
    void reserve(qsizetype size);

    QList<Contact *> search(const QString &searchString, bool includeBlocked = false) const;
    Contact *lookupBySipUrl(const QString &sipUrl) const;
    Contact *lookupByNumber(const QString &number) const;
    Contact *lookupByContactId(const QString &contactId) const;
    Contact *lookupBySourceUid(const QString &sourceUid) const;

    void clear();
    QString hashifyCn(const QString &cn) const;

    const QList<Contact::ContactSourceInfo> &sortedSourceInfos() const
    {
        return m_contactSourceInfos;
    }

private:
    explicit AddressBook(QObject *parent = nullptr);

    QHash<QString, Contact *> m_contacts;
    QHash<QString, Contact *> m_contactsBySourceId;
    QList<Contact::ContactSourceInfo> m_contactSourceInfos;

    QMutex m_feederMutex;

private Q_SLOTS:
    void updateSourceInfos(const Contact *contact);

Q_SIGNALS:
    void contactAdded(Contact *contact);
    void contactModified(Contact *contact);
    void contactRemoved(QString sourceUid);
    void contactsCleared();
    void contactsReady();
    void contactSourceInfosChanged();
};
