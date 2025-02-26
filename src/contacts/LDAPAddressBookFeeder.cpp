#include <ldap.h>
#include <sasl/sasl.h>
#include <QDebug>
#include <QLoggingCategory>

#include "LDAPAddressBookFeeder.h"
#include "AddressBook.h"
#include "ErrorBus.h"

Q_LOGGING_CATEGORY(lcLDAPAddressBookFeeder, "gonnect.app.feeder.LDAPAddressBookFeeder")

LDAPAddressBookFeeder::LDAPAddressBookFeeder(bool useSSL, const QString &ldapUrl,
                                             const QString &ldapBase, const QString &ldapFilter,
                                             const BindMethod bindMethod, const QString &bindDn,
                                             const QString &bindPassword, const QString &saslRealm,
                                             const QString &saslAuthcid, const QString &saslAuthzid,
                                             const QString &caFilePath,
                                             QStringList sipStatusSubscriptableAttributes,
                                             const QString &baseNumber, QObject *parent)
    : QObject{ parent },
      m_useSSL{ useSSL },
      m_bindMethod{ bindMethod },
      m_bindDn{ bindDn },
      m_bindPassword{ bindPassword },
      m_ldapUrl{ ldapUrl },
      m_ldapBase{ ldapBase },
      m_ldapFilter{ ldapFilter },
      m_baseNumber{ baseNumber },
      m_saslRealm{ saslRealm },
      m_saslAuthcid{ saslAuthcid },
      m_saslAuthzid{ saslAuthzid },
      m_caFilePath{ caFilePath },
      m_sipStatusSubscriptableAttributes{ sipStatusSubscriptableAttributes }
{
}

void LDAPAddressBookFeeder::clearCStringlist(char **attrs) const
{
    char **p;

    for (p = attrs; *p; p++) {
        free(*p);
    }

    free(attrs);
}

struct SaslDefaults
{
    LDAP *ldap = nullptr;
    QString mech;
    QString realm;
    QString authcid;
    QString passwd;
    QString authzid;
};

static int interact(LDAP *ld, unsigned flags, void *defaults, void *sasl_interact)
{
    Q_UNUSED(ld)
    Q_UNUSED(flags)

    const auto saslDefaults = static_cast<SaslDefaults *>(defaults);
    auto data = static_cast<sasl_interact_t *>(sasl_interact);

    std::string resultStr;

    switch (data->id) {
    case SASL_CB_GETREALM:
        resultStr = saslDefaults->realm.toStdString();
        break;

    case SASL_CB_AUTHNAME:
        resultStr = saslDefaults->authcid.toStdString();
        break;

    case SASL_CB_PASS:
        resultStr = saslDefaults->passwd.toStdString();
        break;

    case SASL_CB_USER: {
        resultStr = saslDefaults->authzid.toStdString();
        break;
    }

    // Do nothing
    case SASL_CB_NOECHOPROMPT:
    case SASL_CB_ECHOPROMPT:
    case SASL_CB_LIST_END:
    default:
        break;
    }

    const char *cstr = resultStr.c_str();
    data->len = strlen(cstr);
    data->result = malloc(data->len * sizeof(char));
    strlcpy(const_cast<char *>(static_cast<const char *>(data->result)), cstr, data->len);

    qCDebug(lcLDAPAddressBookFeeder)
            << "SASL interactive method response:" << data->id << resultStr;

    return LDAP_SUCCESS;
}

void LDAPAddressBookFeeder::feedAddressBook(AddressBook &addressBook)
{
    char *a = nullptr;
    char *dnTemp = nullptr;
    BerElement *ber = nullptr;
    struct berval **vals;
    char *matchedMsg = nullptr;
    char *errorMsg = nullptr;
    int numEntries = 0, numRefs = 0, result = 0, msgType = 0, parseResultCode = 0;
    LDAP *ldap = nullptr;
    LDAPMessage *msg = nullptr;

    QStringList attributes = { "cn",        "o",    "telephoneNumber", "mobile",
                               "homePhone", "mail", "modifyTimestamp" };

    size_t i = 0;
    char **attrs = (char **)malloc((attributes.count() + 1) * sizeof(char *));
    for (auto &attr : std::as_const(attributes)) {
        size_t sz = attr.size() + 1;
        char *p = (char *)malloc(sz);
        strncpy(p, attr.toLocal8Bit().toStdString().c_str(), sz);
        attrs[i++] = p;
    }
    attrs[i] = NULL;

    QString dn, cn, company, number, mail, modifyTimestamp;
    QList<Contact::PhoneNumber> phoneNumbers;

    qCInfo(lcLDAPAddressBookFeeder) << "Connecting to LDAP service";

    // LDAP debug level
    // const int dbgLvl = 0xFFFF;
    // ldap_set_option(ldap, LDAP_OPT_DEBUG_LEVEL, &dbgLvl);

    if (!m_caFilePath.isEmpty()) {
        qCInfo(lcLDAPAddressBookFeeder) << "Setting custom TLS file for LDAP:" << m_caFilePath;
        ldap_set_option(ldap, LDAP_OPT_X_TLS_CACERTFILE, m_caFilePath.toStdString().c_str());
    }

    result = ldap_initialize(&ldap, m_ldapUrl.toStdString().c_str());
    if (result != LDAP_SUCCESS) {
        qCCritical(lcLDAPAddressBookFeeder)
                << "Could not initialize LDAP handle from uri:" << ldap_err2string(result);
        ErrorBus::instance().addError(
                tr("Failed to initialize LDAP connection: %1").arg(ldap_err2string(result)));
        clearCStringlist(attrs);
        return;
    }

    // Use LDAP version 3
    int version = 3;
    ldap_set_option(ldap, LDAP_OPT_PROTOCOL_VERSION, &version);

    // Use TLS
    if (m_useSSL) {
        result = ldap_start_tls_s(ldap, NULL, NULL);

        if (result != LDAP_SUCCESS) {
            qCCritical(lcLDAPAddressBookFeeder)
                    << "Could not start TLS for LDAP:" << ldap_err2string(result);
            ErrorBus::instance().addError(
                    tr("Failed to initialize LDAP connection: %1").arg(ldap_err2string(result)));
            clearCStringlist(attrs);
            return;
        }

        qCDebug(lcLDAPAddressBookFeeder)
                << "TLS installed on LDAP handle:" << ldap_tls_inplace(ldap);
    }

    // LDAP bind
    if (m_bindMethod != BindMethod::None) {
        if (m_bindMethod == BindMethod::Simple) {
            berval cred;
            const std::string pw = m_bindPassword.toStdString();
            cred.bv_val = (char *)pw.c_str();
            cred.bv_len = m_bindPassword.length();

            qCDebug(lcLDAPAddressBookFeeder)
                    << "LDAP simple bind vs." << m_ldapUrl << "with" << m_bindDn;

            result = ldap_sasl_bind_s(ldap, m_bindDn.toStdString().c_str(), LDAP_SASL_SIMPLE, &cred,
                                      NULL, NULL, NULL);

        } else if (m_bindMethod == BindMethod::GSSAPI) {
            SaslDefaults defaults;
            defaults.ldap = ldap;
            defaults.mech = "GSSAPI";
            defaults.realm = m_saslRealm;
            defaults.authcid = m_saslAuthcid;
            defaults.passwd = m_bindPassword;
            defaults.authzid = m_saslAuthzid;
            result = ldap_sasl_interactive_bind_s(ldap, m_bindDn.toStdString().c_str(), "GSSAPI",
                                                  NULL, NULL, LDAP_SASL_INTERACTIVE, &interact,
                                                  &defaults);
        }

        if (result != LDAP_SUCCESS) {
            qCCritical(lcLDAPAddressBookFeeder) << "Error on LDAP bind:" << ldap_err2string(result);
            ErrorBus::instance().addError(tr("LDAP error: %1").arg(ldap_err2string(result)));
            clearCStringlist(attrs);
            return;
        }

        qCDebug(lcLDAPAddressBookFeeder) << "LDAP bind successful";
    }

    timeval timeout;
    timeout.tv_sec = 3;
    timeout.tv_usec = 0;

    result = ldap_search_ext_s(ldap, m_ldapBase.toLocal8Bit().data(), LDAP_SCOPE_SUBTREE,
                               m_ldapFilter.toStdString().c_str(), attrs, false, NULL, NULL,
                               &timeout, 0, &msg);

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

                ldap_memfree(a);
            }

            addressBook.addContact(dn, cn, company, mail,
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

    ldap_unbind_ext(ldap, NULL, NULL);
}
