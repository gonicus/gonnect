#ifdef WIN32
#  include <winsock2.h>
#endif

#include <ldap.h>
#include <QDebug>
#include <QLoggingCategory>

#include "LDAPAddressBookFeeder.h"
#include "AddressBook.h"
#include "AddressBookManager.h"
#include "ErrorBus.h"
#include "ReadOnlyConfdSettings.h"
#include "AvatarManager.h"
#include "AddressBookManager.h"

Q_LOGGING_CATEGORY(lcLDAPAddressBookFeeder, "gonnect.app.feeder.LDAPAddressBookFeeder")

LDAPAddressBookFeeder::LDAPAddressBookFeeder(const QString &group, AddressBookManager *parent)
    : QObject(parent), m_group(group)
{
    m_manager = qobject_cast<AddressBookManager *>(parent);
}

void LDAPAddressBookFeeder::init(const LDAPInitializer::Config &ldapConfig,
                                 QStringList sipStatusSubscriptableAttributes,
                                 const QString &baseNumber)
{

    m_ldapConfig = ldapConfig;
    m_baseNumber = baseNumber;
    m_sipStatusSubscriptableAttributes = sipStatusSubscriptableAttributes;
}

void LDAPAddressBookFeeder::process()
{
    ReadOnlyConfdSettings settings;

    settings.beginGroup(m_group);

    m_displayName = settings.value("displayName", "").toString();
    bool ok = true;
    m_priority = settings.value("prio", 0).toUInt(&ok);
    if (!ok) {
        qCWarning(lcLDAPAddressBookFeeder) << "Could not parse priority value for" << m_group;
        m_priority = 0;
    }

    const auto bindMethodStr = settings.value("bindMethod", "none").toString();

    if (bindMethodStr == "simple" || bindMethodStr == "gssapi") {
        m_manager->acquireSecret(m_group,
                                 [this](const QString &password) { processImpl(password); });

    } else { // "none"
        processImpl("");
    }
}

void LDAPAddressBookFeeder::processImpl(const QString &password)
{
    ReadOnlyConfdSettings settings;

    QString url = settings.value(m_group + "/url", "").toString();

    settings.beginGroup(m_group);
    const auto scriptableAttributes =
            settings.value("sipStatusSubscriptableAttributes", "").toString();

    const auto bindMethodStr = settings.value("bindMethod", "none").toString();
    LDAPInitializer::BindMethod bindMethod;

    static const QHash<QString, LDAPInitializer::BindMethod> s_bindMethods = {
        { "none", LDAPInitializer::BindMethod::None },
        { "simple", LDAPInitializer::BindMethod::Simple },
        { "gssapi", LDAPInitializer::BindMethod::GSSAPI },
    };

    if (s_bindMethods.contains(bindMethodStr)) {
        bindMethod = s_bindMethods.value(bindMethodStr);
    } else {
        qCCritical(lcLDAPAddressBookFeeder).nospace()
                << "Unknown LDAP bind method '" << bindMethodStr
                << "' - initialization of LDAP account will be aborted.";
        return;
    }

    LDAPInitializer::Config ldapConfig;
    ldapConfig.useSSL = settings.value("useSSL", false).toBool();
    ldapConfig.bindMethod = bindMethod;
    ldapConfig.caFilePath = settings.value("caFile", "").toString();
    ldapConfig.ldapUrl = url;
    ldapConfig.ldapBase = settings.value("base", "").toString();
    ldapConfig.ldapFilter = settings.value("filter", "").toString();
    ldapConfig.bindDn = settings.value("bindDn", "").toString();
    ldapConfig.bindPassword = password;
    ldapConfig.saslRealm = settings.value("realm", "").toString();
    ldapConfig.saslAuthcid = settings.value("authcid", "").toString();
    ldapConfig.saslAuthzid = settings.value("authzid", "").toString();

    init(ldapConfig,
         scriptableAttributes.isEmpty() ? QStringList() : scriptableAttributes.split(QChar(',')),
         settings.value("baseNumber", "").toString());

    feedAddressBook();

    AvatarManager::instance().initialLoad(ldapConfig);

    settings.endGroup();
}

void LDAPAddressBookFeeder::clearCStringlist(char **attrs) const
{
    char **p;

    for (p = attrs; *p; p++) {
        free(*p);
    }

    free(attrs);
}

void LDAPAddressBookFeeder::feedAddressBook()
{
    auto &addressBook = AddressBook::instance();

    char *a = nullptr;
    char *dnTemp = nullptr;
    BerElement *ber = nullptr;
    struct berval **vals;
    char *matchedMsg = nullptr;
    char *errorMsg = nullptr;
    int numEntries = 0, numRefs = 0, result = 0, msgType = 0, parseResultCode = 0;
    LDAP *ldap = nullptr;
    LDAPMessage *msg = nullptr;

    QStringList attributes = { "uid",    "cn",        "o",    "telephoneNumber",
                               "mobile", "homePhone", "mail", "modifyTimestamp" };

    size_t i = 0;
    char **attrs = (char **)malloc((attributes.count() + 1) * sizeof(char *));
    for (auto &attr : std::as_const(attributes)) {
        size_t sz = attr.size() + 1;
        char *p = (char *)malloc(sz);
        strncpy(p, attr.toLocal8Bit().toStdString().c_str(), sz);
        attrs[i++] = p;
    }
    attrs[i] = NULL;

    QString dn, cn, sourceUid, company, number, mail, modifyTimestamp;
    QList<Contact::PhoneNumber> phoneNumbers;

    ldap = LDAPInitializer::initialize(m_ldapConfig);
    if (!ldap) {
        qCCritical(lcLDAPAddressBookFeeder) << "Could not get LDAP connection - aborting";
        ErrorBus::instance().addError(tr("Failed to initialize LDAP connection"));
        clearCStringlist(attrs);
        return;
    }

    timeval timeout;
    timeout.tv_sec = 3;
    timeout.tv_usec = 0;

    result = ldap_search_ext_s(ldap, m_ldapConfig.ldapBase.toLocal8Bit().data(), LDAP_SCOPE_SUBTREE,
                               m_ldapConfig.ldapFilter.toStdString().c_str(), attrs, false, NULL,
                               NULL, &timeout, 0, &msg);

    clearCStringlist(attrs);

    if (result != LDAP_SUCCESS) {
        qCCritical(lcLDAPAddressBookFeeder)
                << "Error on search request: " << ldap_err2string(result);
        ErrorBus::instance().addError(tr("LDAP error: %1").arg(ldap_err2string(result)));
        return;
    }

    numEntries = ldap_count_entries(ldap, msg);
    numRefs = ldap_count_references(ldap, msg);

    qCInfo(lcLDAPAddressBookFeeder)
            << "Retrieved" << numEntries << "entries and" << numRefs << "refs";

    for (msg = ldap_first_message(ldap, msg); msg != NULL; msg = ldap_next_message(ldap, msg)) {

        msgType = ldap_msgtype(msg);

        switch (msgType) {
        case LDAP_RES_SEARCH_ENTRY: {
            dn = "";
            cn = "";
            company = "";
            number = "";
            mail = "";
            modifyTimestamp = "";
            phoneNumbers.clear();

            // Iterate over attributes
            for (a = ldap_first_attribute(ldap, msg, &ber); a != NULL;
                 a = ldap_next_attribute(ldap, msg, ber)) {

                // Get DN
                if ((dnTemp = ldap_get_dn(ldap, msg)) != NULL) {
                    dn = dnTemp;
                    ldap_memfree(dnTemp);
                }

                // Iterate over values
                if ((vals = ldap_get_values_len(ldap, msg, a)) != NULL) {

                    const auto val = (**vals).bv_val;

                    if (strcmp(a, "cn") == 0) {
                        cn = val;
                    } else if (strcmp(a, "o") == 0) {
                        company = val;
                    } else if (strcmp(a, "uid") == 0) {
                        sourceUid = val;
                    } else if (strcmp(a, "mail") == 0) {
                        mail = val;
                    } else if (strcmp(a, "modifyTimestamp") == 0) {
                        modifyTimestamp = val;
                    } else if (strcmp(a, "telephoneNumber") == 0) {
                        number = val;
                        if (number.startsWith(m_baseNumber)) {
                            number = number.sliced(QString(m_baseNumber).size());
                        }
                        phoneNumbers.append(
                                { Contact::NumberType::Commercial, number,
                                  m_sipStatusSubscriptableAttributes.contains("telephoneNumber") });
                    } else if (strcmp(a, "mobile") == 0) {
                        number = val;
                        if (number.startsWith(m_baseNumber)) {
                            number = number.sliced(QString(m_baseNumber).size());
                        }
                        phoneNumbers.append(
                                { Contact::NumberType::Mobile, number,
                                  m_sipStatusSubscriptableAttributes.contains("mobile") });
                    } else if (strcmp(a, "homePhone") == 0) {
                        number = val;
                        if (number.startsWith(m_baseNumber)) {
                            number = number.sliced(QString(m_baseNumber).size());
                        }
                        phoneNumbers.append(
                                { Contact::NumberType::Home, number,
                                  m_sipStatusSubscriptableAttributes.contains("homePhone") });
                    }

                    ldap_value_free_len(vals);
                }
            }

            addressBook.addContact(dn, sourceUid, { m_priority, m_displayName }, cn, company, mail,
                                   QDateTime::fromString(modifyTimestamp, "yyyyMMddhhmmsst"),
                                   phoneNumbers);

            a = nullptr;

            ber_free(ber, 0);
            ber = nullptr;

            break;
        }

        case LDAP_RES_SEARCH_RESULT: {
            parseResultCode =
                    ldap_parse_result(ldap, msg, &result, &matchedMsg, &errorMsg, NULL, NULL, 1);
            if (parseResultCode != LDAP_SUCCESS) {
                qCCritical(lcLDAPAddressBookFeeder)
                        << "LDAP parse error:" << ldap_err2string(parseResultCode);
                ErrorBus::instance().addError(
                        tr("Parse error: %1").arg(ldap_err2string(parseResultCode)));
                return;
            }
            break;
        }

        default:
            qCCritical(lcLDAPAddressBookFeeder) << "Unknown message type:" << msgType;
            return;
        }
    }

    LDAPInitializer::freeLDAPHandle(ldap);
}

QUrl LDAPAddressBookFeeder::networkCheckURL() const
{
    ReadOnlyConfdSettings settings;

    QString url = settings.value(m_group + "/url", "").toString();
    return QUrl(url);
}
