#pragma once

#include <evolution-data-server/libebook/libebook.h>
#include <evolution-data-server/libedata-book/libedata-book.h>
#include <glib.h>

#include <QObject>
#include <QFuture>
#include <QFutureWatcher>
#include <QPromise>

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
    QString getFieldMerge(EContact *contact, EContactField pId, EContactField sId);
    QStringList getList(EContact *contact, EContactField id);
    void addAvatar(QString id, EContact *contact, QDateTime changed);

    void init();
    void feedAddressBook();

    static void onEbookClientConnected(GObject *source_object, GAsyncResult *result,
                                       gpointer user_data);

    void connectContactSignals(EBookClientView *view);

    static void onContactsAdded(EBookClientView *view, GSList *contacts, gpointer user_data);
    static void onContactsModified(EBookClientView *view, GSList *contacts, gpointer user_data);
    static void onContactsRemoved(EBookClientView *view, GSList *uids, gpointer user_data);

    void processContactsAdded(GSList *contacts);
    void processContactsModified(GSList *contacts);
    void processContactsRemoved(GSList *uids);

    static void onViewCreated(GObject *source_object, GAsyncResult *result, gpointer user_data);

    static void onClientContactsRequested(GObject *source_object, GAsyncResult *result,
                                          gpointer user_data);

    void processContacts(QString clientInfo, GSList *contacts);

    QString m_group;
    bool m_block = false;

    ESourceRegistry *m_registry = nullptr;
    GList *m_sources = nullptr;
    gchar *m_searchExpr = nullptr;
    QList<EBookClient *> m_clients;
    QList<EBookClientView *> m_clientViews;

    int m_sourceCount = 0;
    std::atomic<int> m_clientCount = 0;
    QPromise<void> *m_sourcePromise = nullptr;
    QFuture<void> m_sourceFuture;
    QFutureWatcher<void> *m_futureWatcher = nullptr;
};
