#include "EDSAddressBookFeeder.h"
#include "Contact.h"
#include "AddressBook.h"
#include "AddressBookManager.h"
#include "AvatarManager.h"
#include "ReadOnlyConfdSettings.h"

#include <QBuffer>
#include <QImage>
#include <QLoggingCategory>
#include <QStringList>

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
}

bool EDSAddressBookFeeder::init()
{
    GError *error = nullptr;

    // Create a source registry
    m_registry = e_source_registry_new_sync(nullptr, &error);
    if (!m_registry) {
        qCDebug(lcEDSAddressBookFeeder) << "Can't create registry: " << error->message;
        g_error_free(error);
        error = nullptr;
        return false;
    }

    // Get the enabled address book sources of the registry
    m_sources = e_source_registry_list_enabled(m_registry, E_SOURCE_EXTENSION_ADDRESS_BOOK);
    if (g_list_length(m_sources) == 0) {
        qCDebug(lcEDSAddressBookFeeder) << "No sources found in registry";
        return false;
    }

    // Prepare the search query
    EBookQuery *query = e_book_query_any_field_contains("");
    m_searchExpr = e_book_query_to_string(query);
    e_book_query_unref(query);

    // Clients and signals
    EBookClient *client = nullptr;
    for (GList *iter = m_sources; iter != nullptr; iter = g_list_next(iter)) {
        ESource *source = E_SOURCE(iter->data);

        const gchar *id = e_source_get_uid(source);
        const gchar *dn = e_source_get_display_name(source);

        client = E_BOOK_CLIENT(e_book_client_connect_sync(source, 5, nullptr, &error));
        if (!client) {
            qCDebug(lcEDSAddressBookFeeder) << "Can't connect to '" << id << "' (" << dn << "): '"
                                            << error->message << "', skipping...";
            g_error_free(error);
            error = nullptr;
        } else {
            qCDebug(lcEDSAddressBookFeeder) << "Connected to '" << id << "' (" << dn << ")";

            // INFO: e_book_client_get_view(): The last argument is an instance of the EDS class
            // (gpointer user_data) client -> e_book_client_get_view() -> e_book_client_get_finish()
            // -> e_book_client_view_start()
            e_book_client_get_view(client, m_searchExpr, nullptr, onViewCreated, this);
            m_clients.append(client);
        }
    }

    return true;
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

    m_sipStatusSubscriptableFields.clear();
    const auto sipFieldsValue = settings.value("sipStatusSubscriptableFields", "").toString();
    if (!sipFieldsValue.isEmpty()) {
        const auto tokens = sipFieldsValue.split(',', Qt::SkipEmptyParts);
        for (const auto &token : tokens) {
            const auto trimmedToken = token.trimmed();
            const auto normalizedToken = trimmedToken.toLower();

            if (normalizedToken == "company") {
                m_sipStatusSubscriptableFields.insert(E_CONTACT_PHONE_COMPANY);
                m_sipStatusSubscriptableFields.insert(E_CONTACT_PHONE_BUSINESS);
            } else if (normalizedToken == "business") {
                m_sipStatusSubscriptableFields.insert(E_CONTACT_PHONE_BUSINESS);
            } else if (normalizedToken == "mobile") {
                m_sipStatusSubscriptableFields.insert(E_CONTACT_PHONE_MOBILE);
            } else if (normalizedToken == "home") {
                m_sipStatusSubscriptableFields.insert(E_CONTACT_PHONE_HOME);
            } else if (!trimmedToken.isEmpty()) {
                qCWarning(lcEDSAddressBookFeeder).nospace()
                        << "Unknown sipStatusSubscriptableFields token '" << trimmedToken
                        << "' for group '" << m_group << "'";
            }
        }
    }

    if (init()) {
        feedAddressBook();
    }

    settings.endGroup();
}

QString EDSAddressBookFeeder::getField(EContact *contact, EContactField id)
{
    if (contact) {
        return QString::fromUtf8(static_cast<const gchar *>(e_contact_get_const(contact, id)));
    }

    return "";
}

QList<Contact::PhoneNumber> EDSAddressBookFeeder::collectPhoneNumbers(EContact *contact) const
{
    QList<Contact::PhoneNumber> phoneNumbers;

    const auto companyNumber = getField(contact, E_CONTACT_PHONE_COMPANY);
    const auto businessNumber = getField(contact, E_CONTACT_PHONE_BUSINESS);
    const auto mobileNumber = getField(contact, E_CONTACT_PHONE_MOBILE);
    const auto homeNumber = getField(contact, E_CONTACT_PHONE_HOME);

    const auto commercialNumber = !companyNumber.isEmpty() ? companyNumber : businessNumber;
    const bool commercialSubscriptable =
            (!companyNumber.isEmpty() && isSipStatusSubscriptable(E_CONTACT_PHONE_COMPANY))
            || (!businessNumber.isEmpty() && isSipStatusSubscriptable(E_CONTACT_PHONE_BUSINESS));

    phoneNumbers.append({ Contact::NumberType::Commercial, commercialNumber,
                          commercialSubscriptable });

    phoneNumbers.append({ Contact::NumberType::Mobile, mobileNumber,
                          !mobileNumber.isEmpty()
                                  && isSipStatusSubscriptable(E_CONTACT_PHONE_MOBILE) });

    phoneNumbers.append({ Contact::NumberType::Home, homeNumber,
                          !homeNumber.isEmpty()
                                  && isSipStatusSubscriptable(E_CONTACT_PHONE_HOME) });

    return phoneNumbers;
}

bool EDSAddressBookFeeder::isSipStatusSubscriptable(EContactField field) const
{
    return m_sipStatusSubscriptableFields.contains(field);
}

void EDSAddressBookFeeder::connectContactSignals(EBookClientView *view)
{
    // INFO: g_signal_connect(): The last argument is an instance of the EDS class (gpointer
    // user_data)
    g_signal_connect(view, "objects-added", G_CALLBACK(onContactsAdded), this);
    g_signal_connect(view, "objects-modified", G_CALLBACK(onContactsModified), this);
    g_signal_connect(view, "objects-removed", G_CALLBACK(onContactsRemoved), this);
}

void EDSAddressBookFeeder::onContactsAdded(EBookClient *client, GSList *contacts,
                                           gpointer user_data)
{
    EDSAddressBookFeeder *feeder = static_cast<EDSAddressBookFeeder *>(user_data);
    if (feeder) {
        feeder->processContactsAdded(client, contacts);
    }
}

void EDSAddressBookFeeder::onContactsModified(EBookClient *client, GSList *contacts,
                                              gpointer user_data)
{
    EDSAddressBookFeeder *feeder = static_cast<EDSAddressBookFeeder *>(user_data);
    if (feeder) {
        feeder->processContactsModified(client, contacts);
    }
}

void EDSAddressBookFeeder::onContactsRemoved(EBookClient *client, GSList *uids, gpointer user_data)
{
    EDSAddressBookFeeder *feeder = static_cast<EDSAddressBookFeeder *>(user_data);
    if (feeder) {
        feeder->processContactsRemoved(client, uids);
    }
}

void EDSAddressBookFeeder::processContactsAdded(EBookClient *client, GSList *contacts)
{
    Q_UNUSED(client)

    auto &addressbook = AddressBook::instance();

    for (GSList *item = contacts; item != nullptr; item = g_slist_next(item)) {
        if (EContact *eContact = E_CONTACT(item->data)) {
            QDateTime changed =
                    QDateTime::fromString(getField(eContact, E_CONTACT_REV), Qt::ISODate);

            QList<Contact::PhoneNumber> phoneNumbers;
            phoneNumbers = collectPhoneNumbers(eContact);

            Contact *contact = addressbook.addContact(
                    getField(eContact, E_CONTACT_FULL_NAME) + getField(eContact, E_CONTACT_ORG),
                    getField(eContact, E_CONTACT_UID), { m_priority, m_displayName },
                    getField(eContact, E_CONTACT_FULL_NAME), getField(eContact, E_CONTACT_ORG),
                    getField(eContact, E_CONTACT_EMAIL_1), changed, phoneNumbers);

            addAvatar(contact->id(), eContact, changed);
        }
    }
}

void EDSAddressBookFeeder::processContactsModified(EBookClient *client, GSList *contacts)
{
    Q_UNUSED(client)

    auto &addressbook = AddressBook::instance();

    for (GSList *item = contacts; item != nullptr; item = g_slist_next(item)) {
        if (EContact *eContact = E_CONTACT(item->data)) {
            QDateTime changed =
                    QDateTime::fromString(getField(eContact, E_CONTACT_REV), Qt::ISODate);

            QList<Contact::PhoneNumber> phoneNumbers = collectPhoneNumbers(eContact);

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

void EDSAddressBookFeeder::processContactsRemoved(EBookClient *client, GSList *uids)
{
    Q_UNUSED(client)

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

void EDSAddressBookFeeder::onViewCreated(GObject *source_object, GAsyncResult *result,
                                         gpointer user_data)
{
    GError *error = nullptr;
    EBookClientView *view = nullptr;

    EDSAddressBookFeeder *feeder = static_cast<EDSAddressBookFeeder *>(user_data);
    if (feeder) {
        if (!e_book_client_get_view_finish(E_BOOK_CLIENT(source_object), result, &view, &error)) {
            qCDebug(lcEDSAddressBookFeeder) << "Can't retrieve finished view: " << error->message;
            g_error_free(error);
            error = nullptr;
            return;
        }

        feeder->m_clientViews.append(view);
        feeder->connectContactSignals(view);
        e_book_client_view_start(view, &error);
        if (error) {
            qCDebug(lcEDSAddressBookFeeder) << "Can't start view: " << error->message;
            g_error_free(error);
            error = nullptr;
        }
    }
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
    unsigned contactCount = 0;
    auto &addressbook = AddressBook::instance();

    GError *error = nullptr;
    GSList *contacts = nullptr;

    for (auto client : std::as_const(m_clients)) {
        const gchar *id = e_source_get_uid(e_client_get_source(E_CLIENT(client)));
        const gchar *dn = e_source_get_display_name(e_client_get_source(E_CLIENT(client)));

        if (!e_book_client_get_contacts_sync(client, m_searchExpr, &contacts, nullptr, &error)) {
            qCDebug(lcEDSAddressBookFeeder)
                    << "Can't get contacts of '" << id << "' (" << dn << "), skipping...";
            g_error_free(error);
            error = nullptr;
        } else {
            for (GSList *item = contacts; item != nullptr; item = g_slist_next(item)) {
                if (EContact *eContact = E_CONTACT(item->data)) {
                    QDateTime changed =
                            QDateTime::fromString(getField(eContact, E_CONTACT_REV), Qt::ISODate);

                    QList<Contact::PhoneNumber> phoneNumbers = collectPhoneNumbers(eContact);

                    Contact *contact = addressbook.addContact(
                            getField(eContact, E_CONTACT_FULL_NAME)
                                    + getField(eContact, E_CONTACT_ORG),
                            getField(eContact, E_CONTACT_UID), { m_priority, m_displayName },
                            getField(eContact, E_CONTACT_FULL_NAME),
                            getField(eContact, E_CONTACT_ORG),
                            getField(eContact, E_CONTACT_EMAIL_1), changed, phoneNumbers);

                    addAvatar(contact->id(), eContact, changed);

                    contactCount++;
                }
            }

            qCDebug(lcEDSAddressBookFeeder) << "Loaded contacts of '" << id << "' (" << dn << ")";

            g_slist_free_full(contacts, g_object_unref);
            contacts = nullptr;
        }
    }

    qCInfo(lcEDSAddressBookFeeder)
            << "Finished processing EDS sources, loaded" << contactCount << "contacts";
}
