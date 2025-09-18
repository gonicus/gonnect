#pragma once

#include <evolution-data-server/libebook/libebook.h>
#include <evolution-data-server/libedata-book/libedata-book.h>
#include <glib.h>

#include <QObject>
#include <QSet>
#include "Contact.h"
#include "IAddressBookFeeder.h"

class AddressBookManager;

class EDSAddressBookFeeder : public QObject, public IAddressBookFeeder
{
    Q_OBJECT

public:
    explicit EDSAddressBookFeeder(const QString &group, AddressBookManager *parent = nullptr);
    ~EDSAddressBookFeeder();

    void process() override;

private:
    QString getField(EContact *contact, EContactField id);
    void addAvatar(QString id, EContact *contact, QDateTime changed);
    QList<Contact::PhoneNumber> collectPhoneNumbers(EContact *contact) const;
    bool isSipStatusSubscriptable(EContactField field) const;

    bool init();
    void feedAddressBook();

    void connectContactSignals(EBookClientView *view);

    static void onContactsAdded(EBookClient *client, GSList *contacts, gpointer user_data);
    static void onContactsModified(EBookClient *client, GSList *contacts, gpointer user_data);
    static void onContactsRemoved(EBookClient *client, GSList *uids, gpointer user_data);

    void processContactsAdded(EBookClient *client, GSList *contacts);
    void processContactsModified(EBookClient *client, GSList *contacts);
    void processContactsRemoved(EBookClient *client, GSList *uids);

    static void onViewCreated(GObject *source_object, GAsyncResult *result, gpointer user_data);

    QString m_group;

    ESourceRegistry *m_registry = nullptr;
    GList *m_sources = nullptr;
    gchar *m_searchExpr = nullptr;
    QList<EBookClient *> m_clients;
    QList<EBookClientView *> m_clientViews;
    QSet<EContactField> m_sipStatusSubscriptableFields;
};
