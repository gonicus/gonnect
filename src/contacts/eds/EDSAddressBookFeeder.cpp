#include "EDSAddressBookFeeder.h"
#include "Contact.h"
#include "AddressBook.h"
#include "AddressBookManager.h"
#include "AvatarManager.h"
#include "ReadOnlyConfdSettings.h"

#include <QBuffer>
#include <QImage>
#include <QLoggingCategory>

using namespace std::chrono_literals;

Q_LOGGING_CATEGORY(lcEDSAddressBookFeeder, "gonnect.app.feeder.EDSAddressBookFeeder")

EDSAddressBookFeeder::EDSAddressBookFeeder(const QString &group, AddressBookManager *parent)
    : QObject(parent), m_group(group)
{
}

EDSAddressBookFeeder::~EDSAddressBookFeeder()
{
    if (m_registry) {
        g_object_unref(m_registry);
    }
    if (m_sources) {
        g_list_free_full(m_sources, g_object_unref);
    }
    if (m_searchExpr) {
        g_free(m_searchExpr);
    }

    for (auto client : std::as_const(m_clients)) {
        g_object_unref(client);
        client = nullptr;
    }

    for (auto clientView : std::as_const(m_clientViews)) {
        g_object_unref(clientView);
        clientView = nullptr;
    }

    if (m_sourcePromise) {
        delete m_sourcePromise;
        m_sourcePromise = nullptr;
    }

    if (m_futureWatcher) {
        m_futureWatcher->deleteLater();
        m_futureWatcher = nullptr;
    }
}

void EDSAddressBookFeeder::init()
{
    GError *error = nullptr;

    // Create a source registry
    m_registry = e_source_registry_new_sync(nullptr, &error);
    if (!m_registry) {
        if (error) {
            qCDebug(lcEDSAddressBookFeeder) << "Can't create registry:" << error->message;
            g_error_free(error);
            error = nullptr;
        }
        return;
    }

    // Get the enabled address book sources of the registry
    m_sources = e_source_registry_list_enabled(m_registry, E_SOURCE_EXTENSION_ADDRESS_BOOK);
    m_sourceCount = g_list_length(m_sources);
    if (m_sourceCount == 0) {
        qCDebug(lcEDSAddressBookFeeder) << "No sources found in registry";
        return;
    }

    // Prepare the search query
    EBookQuery *query = e_book_query_any_field_contains("");
    m_searchExpr = e_book_query_to_string(query);
    e_book_query_unref(query);

    // Clients and signals
    m_sourcePromise = new QPromise<void>();
    m_sourceFuture = m_sourcePromise->future();
    m_futureWatcher = new QFutureWatcher<void>();

    for (GList *iter = m_sources; iter != nullptr; iter = g_list_next(iter)) {
        ESource *source = E_SOURCE(iter->data);

        QString sourceInfo =
                QString("%1 (%2)").arg(e_source_get_display_name(source), e_source_get_uid(source));

        qCDebug(lcEDSAddressBookFeeder) << "Connecting to source" << sourceInfo;

        e_book_client_connect(source, 5, nullptr, onEbookClientConnected, this);
    }

    m_sourcePromise->start();

    QtFuture::connect(m_futureWatcher, &QFutureWatcher<void>::finished).then([this]() {
        if (m_sourceFuture.isFinished()) {
            feedAddressBook();
        }
    });

    QTimer::singleShot(5s, this, [this]() {
        if (!m_futureWatcher->isFinished()) {
            qCDebug(lcEDSAddressBookFeeder) << "Failed to process EDS sources";

            m_sourceFuture.cancel();
            m_futureWatcher->cancel();
        }
    });

    m_futureWatcher->setFuture(m_sourceFuture);
}

void EDSAddressBookFeeder::process()
{
    ReadOnlyConfdSettings settings;

    settings.beginGroup(m_group);
    m_displayName = settings.value("displayName", "").toString();
    bool ok = true;
    m_priority = settings.value("prio", 0).toUInt(&ok);
    if (!ok) {
        qCWarning(lcEDSAddressBookFeeder) << "Could not parse priority value for" << m_group;
        m_priority = 0;
    }
    settings.endGroup();

    init();
}

QString EDSAddressBookFeeder::getField(EContact *contact, EContactField id)
{
    if (contact) {
        return QString::fromUtf8(static_cast<const gchar *>(e_contact_get_const(contact, id)));
    }

    return "";
}

QString EDSAddressBookFeeder::getFieldMerge(EContact *contact, EContactField pId, EContactField sId)
{
    if (contact) {
        const gchar *field = static_cast<const gchar *>(e_contact_get_const(contact, pId));
        if (!field) {
            field = static_cast<const gchar *>(e_contact_get_const(contact, sId));
        }

        return QString::fromUtf8(field);
    }

    return "";
}

void EDSAddressBookFeeder::connectContactSignals(EBookClientView *view)
{
    // INFO: g_signal_connect(): The last argument is an instance of the EDS class (gpointer
    // user_data)
    g_signal_connect(view, "objects-added", G_CALLBACK(onContactsAdded), this);
    g_signal_connect(view, "objects-modified", G_CALLBACK(onContactsModified), this);
    g_signal_connect(view, "objects-removed", G_CALLBACK(onContactsRemoved), this);
}

void EDSAddressBookFeeder::onContactsAdded(EBookClientView *view, GSList *contacts,
                                           gpointer user_data)
{
    Q_UNUSED(view)

    EDSAddressBookFeeder *feeder = static_cast<EDSAddressBookFeeder *>(user_data);
    if (feeder) {
        feeder->processContactsAdded(contacts);
    }
}

void EDSAddressBookFeeder::onContactsModified(EBookClientView *view, GSList *contacts,
                                              gpointer user_data)
{
    Q_UNUSED(view)

    EDSAddressBookFeeder *feeder = static_cast<EDSAddressBookFeeder *>(user_data);
    if (feeder) {
        feeder->processContactsModified(contacts);
    }
}

void EDSAddressBookFeeder::onContactsRemoved(EBookClientView *view, GSList *uids,
                                             gpointer user_data)
{
    Q_UNUSED(view)

    EDSAddressBookFeeder *feeder = static_cast<EDSAddressBookFeeder *>(user_data);
    if (feeder) {
        feeder->processContactsRemoved(uids);
    }
}

void EDSAddressBookFeeder::processContactsAdded(GSList *contacts)
{
    auto &addressbook = AddressBook::instance();

    for (GSList *item = contacts; item != nullptr; item = g_slist_next(item)) {
        if (EContact *eContact = E_CONTACT(item->data)) {
            QDateTime changed =
                    QDateTime::fromString(getField(eContact, E_CONTACT_REV), Qt::ISODate);

            QList<Contact::PhoneNumber> phoneNumbers;
            phoneNumbers.append(
                    { Contact::NumberType::Commercial,
                      getFieldMerge(eContact, E_CONTACT_PHONE_COMPANY, E_CONTACT_PHONE_BUSINESS),
                      false });
            phoneNumbers.append({ Contact::NumberType::Mobile,
                                  getField(eContact, E_CONTACT_PHONE_MOBILE), false });
            phoneNumbers.append(
                    { Contact::NumberType::Home, getField(eContact, E_CONTACT_PHONE_HOME), false });

            Contact *contact = addressbook.addContact(
                    getField(eContact, E_CONTACT_FULL_NAME) + getField(eContact, E_CONTACT_ORG),
                    getField(eContact, E_CONTACT_UID), { m_priority, m_displayName },
                    getField(eContact, E_CONTACT_FULL_NAME), getField(eContact, E_CONTACT_ORG),
                    getField(eContact, E_CONTACT_EMAIL_1), changed, phoneNumbers);

            addAvatar(contact->id(), eContact, changed);
        }
    }
}

void EDSAddressBookFeeder::processContactsModified(GSList *contacts)
{
    auto &addressbook = AddressBook::instance();

    for (GSList *item = contacts; item != nullptr; item = g_slist_next(item)) {
        if (EContact *eContact = E_CONTACT(item->data)) {
            QDateTime changed =
                    QDateTime::fromString(getField(eContact, E_CONTACT_REV), Qt::ISODate);

            QList<Contact::PhoneNumber> phoneNumbers;
            phoneNumbers.append(
                    { Contact::NumberType::Commercial,
                      getFieldMerge(eContact, E_CONTACT_PHONE_COMPANY, E_CONTACT_PHONE_BUSINESS),
                      false });
            phoneNumbers.append({ Contact::NumberType::Mobile,
                                  getField(eContact, E_CONTACT_PHONE_MOBILE), false });
            phoneNumbers.append(
                    { Contact::NumberType::Home, getField(eContact, E_CONTACT_PHONE_HOME), false });

            Contact *contact = addressbook.modifyContact(
                    getField(eContact, E_CONTACT_FULL_NAME) + getField(eContact, E_CONTACT_ORG),
                    getField(eContact, E_CONTACT_UID), getField(eContact, E_CONTACT_FULL_NAME),
                    getField(eContact, E_CONTACT_ORG), getField(eContact, E_CONTACT_EMAIL_1),
                    changed, phoneNumbers);

            if (contact) {
                addAvatar(contact->id(), eContact, changed);
            }
        }
    }
}

void EDSAddressBookFeeder::processContactsRemoved(GSList *uids)
{
    auto &addressbook = AddressBook::instance();

    for (GSList *item = uids; item != nullptr; item = g_slist_next(item)) {
        const gchar *uid = static_cast<const gchar *>(item->data);
        if (uid) {
            // Do not keep images of deleted contacts
            if (auto contact = AddressBook::instance().lookupBySourceUid(uid)) {
                AvatarManager::instance().removeExternalImage(contact->id());
            }
            addressbook.removeContact(uid);
        }
    }
}

void EDSAddressBookFeeder::onEbookClientConnected(GObject *source_object, GAsyncResult *result,
                                                  gpointer user_data)
{
    Q_UNUSED(source_object)

    GError *error = nullptr;
    EBookClient *client = nullptr;

    EDSAddressBookFeeder *feeder = static_cast<EDSAddressBookFeeder *>(user_data);
    if (feeder) {
        client = E_BOOK_CLIENT(e_book_client_connect_finish(result, &error));
        if (error) {
            qCDebug(lcEDSAddressBookFeeder)
                    << "Can't retrieve finished client connection:" << error->message;
            g_error_free(error);
            error = nullptr;
            return;
        }

        e_book_client_get_view(client, feeder->m_searchExpr, nullptr, onViewCreated, feeder);
    }
}

void EDSAddressBookFeeder::onViewCreated(GObject *source_object, GAsyncResult *result,
                                         gpointer user_data)
{
    GError *error = nullptr;
    EBookClient *client = E_BOOK_CLIENT(source_object);
    EBookClientView *view = nullptr;
    EDSAddressBookFeeder *feeder = static_cast<EDSAddressBookFeeder *>(user_data);

    if (feeder && client) {
        if (!e_book_client_get_view_finish(client, result, &view, &error)) {
            if (error) {
                qCCritical(lcEDSAddressBookFeeder)
                        << "Can't retrieve finished view:" << error->message;
                g_error_free(error);
                error = nullptr;
            }
            return;
        }

        feeder->connectContactSignals(view);
        e_book_client_view_start(view, &error);
        if (error) {
            qCCritical(lcEDSAddressBookFeeder) << "Can't start view:" << error->message;
            g_error_free(error);
            error = nullptr;
            return;
        }
        feeder->m_clientViews.append(view);

        feeder->m_clients.append(client);
        feeder->m_clientCount++;
        if (feeder->m_clientCount == feeder->m_sourceCount) {
            feeder->m_sourcePromise->finish();
        }
    }
}

void EDSAddressBookFeeder::onClientContactsRequested(GObject *source_object, GAsyncResult *result,
                                                     gpointer user_data)
{
    GError *error = nullptr;
    GSList *contacts = nullptr;

    EDSAddressBookFeeder *feeder = static_cast<EDSAddressBookFeeder *>(user_data);
    if (feeder) {
        if (!e_book_client_get_contacts_finish(E_BOOK_CLIENT(source_object), result, &contacts,
                                               &error)) {
            if (error) {
                qCCritical(lcEDSAddressBookFeeder) << "Can't retrieve contacts:" << error->message;
                g_error_free(error);
                error = nullptr;
            }
            return;
        }
    }

    if (contacts) {
        QString sourceInfo = QString("%1 (%2)").arg(
                e_source_get_display_name(e_client_get_source(E_CLIENT(source_object))),
                e_source_get_uid(e_client_get_source(E_CLIENT(source_object))));

        feeder->processContacts(sourceInfo, contacts);
    }
}

void EDSAddressBookFeeder::processContacts(QString clientInfo, GSList *contacts)
{
    unsigned contactCount = 0;
    auto &addressbook = AddressBook::instance();

    for (GSList *item = contacts; item != nullptr; item = g_slist_next(item)) {
        if (EContact *eContact = E_CONTACT(item->data)) {
            QDateTime changed =
                    QDateTime::fromString(getField(eContact, E_CONTACT_REV), Qt::ISODate);

            QList<Contact::PhoneNumber> phoneNumbers;
            phoneNumbers.append(
                    { Contact::NumberType::Commercial,
                      getFieldMerge(eContact, E_CONTACT_PHONE_COMPANY, E_CONTACT_PHONE_BUSINESS),
                      false });
            phoneNumbers.append({ Contact::NumberType::Mobile,
                                  getField(eContact, E_CONTACT_PHONE_MOBILE), false });
            phoneNumbers.append(
                    { Contact::NumberType::Home, getField(eContact, E_CONTACT_PHONE_HOME), false });

            Contact *contact = addressbook.addContact(
                    getField(eContact, E_CONTACT_FULL_NAME) + getField(eContact, E_CONTACT_ORG),
                    getField(eContact, E_CONTACT_UID), { m_priority, m_displayName },
                    getField(eContact, E_CONTACT_FULL_NAME), getField(eContact, E_CONTACT_ORG),
                    getField(eContact, E_CONTACT_EMAIL_1), changed, phoneNumbers);

            addAvatar(contact->id(), eContact, changed);

            contactCount++;
        }
    }

    g_slist_free_full(contacts, g_object_unref);
    contacts = nullptr;

    qCInfo(lcEDSAddressBookFeeder)
            << "Loaded" << contactCount << "contact(s) of source" << clientInfo;
}

void EDSAddressBookFeeder::addAvatar(QString id, EContact *contact, QDateTime changed)
{
    EContactPhoto *photo = (EContactPhoto *)e_contact_get(contact, E_CONTACT_PHOTO);
    if (!photo) {
        photo = (EContactPhoto *)e_contact_get(contact, E_CONTACT_LOGO);
    }

    char localFile[] = "file://";

    if (photo) {
        if (photo->type == E_CONTACT_PHOTO_TYPE_INLINED) {
            QByteArray avatar =
                    QString::fromUtf8(static_cast<const guchar *>(photo->data.inlined.data))
                            .toUtf8();
            if (avatar.size()) {
                AvatarManager::instance().addExternalImage(id, avatar, changed);
            }
        } else if (photo->type == E_CONTACT_PHOTO_TYPE_URI && photo->data.uri && *photo->data.uri) {
            bool isLocal = g_str_has_prefix(photo->data.uri, localFile);
            if (isLocal) {
                QString file = QString::fromUtf8(static_cast<const gchar *>(photo->data.uri));
                file.replace(localFile, "");
                QImage image;
                image.load(file);
                if (!image.isNull()) {
                    QByteArray avatar;
                    QBuffer buffer(&avatar);
                    buffer.open(QIODevice::WriteOnly);
                    // INFO: EDS stores contact photos as PNG ("*.image-2Fpng")
                    image.save(&buffer, "PNG");
                    if (avatar.size()) {
                        AvatarManager::instance().addExternalImage(id, avatar, changed);
                    }
                }
            }
        }

        e_contact_photo_free(photo);
    } else {
        // Remove the old contact photo if it exists
        Contact *gContact = AddressBook::instance().lookupByContactId(id);
        if (gContact->hasAvatar()) {
            AvatarManager::instance().removeExternalImage(id);
        }
    }
}

void EDSAddressBookFeeder::feedAddressBook()
{
    for (auto client : std::as_const(m_clients)) {
        e_book_client_get_contacts(client, m_searchExpr, nullptr, onClientContactsRequested, this);
    }
}
