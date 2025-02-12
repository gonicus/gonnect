#pragma once

#include "Contact.h"

#include <QObject>
#include <QHash>

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

    Contact *addContact(const QString &dn, const QString &name, const QString &company,
                        const QString &mail, const QDateTime &lastModified,
                        const QList<Contact::PhoneNumber> &phoneNumbers);

    void addContact(Contact *contact);

    QHash<QString, Contact *> contacts() const;
    void reserve(qsizetype size);

    QList<Contact *> search(const QString &searchString) const;
    Contact *lookupBySipUrl(const QString &sipUrl) const;
    Contact *lookupByNumber(const QString &number) const;
    Contact *lookupByContactId(const QString &contactId) const;

    void clear();
    QString hashifyCn(const QString &cn) const;

private:
    explicit AddressBook(QObject *parent = nullptr);

    QHash<QString, Contact *> m_contacts;

signals:
    void contactAdded(Contact *contact);
    void contactsCleared();
    void contactsReady();
};
