#include "LDAPInitializer.h"

#include <QLoggingCategory>
#include <QCoreApplication>
#include <sasl/sasl.h>

Q_LOGGING_CATEGORY(lcLDAPInitializer, "gonnect.app.LDAPInitializer")

static int interact(LDAP *ld, unsigned flags, void *defaults, void *sasl_interact)
{
    Q_UNUSED(ld)
    Q_UNUSED(flags)

    const auto saslDefaults = static_cast<LDAPInitializer::Config *>(defaults);
    auto data = static_cast<sasl_interact_t *>(sasl_interact);

    std::string resultStr;

    switch (data->id) {
    case SASL_CB_GETREALM:
        resultStr = saslDefaults->saslRealm.toStdString();
        break;

    case SASL_CB_AUTHNAME:
        resultStr = saslDefaults->saslAuthcid.toStdString();
        break;

    case SASL_CB_PASS:
        resultStr = saslDefaults->bindPassword.toStdString();
        break;

    case SASL_CB_USER: {
        resultStr = saslDefaults->saslAuthzid.toStdString();
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

    qCDebug(lcLDAPInitializer) << "SASL interactive method response:" << data->id << resultStr;

    return LDAP_SUCCESS;
}

LDAP *LDAPInitializer::initialize(const LDAPInitializer::Config &config)
{
    LDAP *ldap = nullptr;
    int result = 0;

    qCInfo(lcLDAPInitializer) << "Connecting to LDAP service";

    // LDAP debug level
    // const int dbgLvl = 0xFFFF;
    // ldap_set_option(ldap, LDAP_OPT_DEBUG_LEVEL, &dbgLvl);

    if (!config.caFilePath.isEmpty()) {
        qCInfo(lcLDAPInitializer) << "Setting custom TLS file for LDAP:" << config.caFilePath;
        ldap_set_option(ldap, LDAP_OPT_X_TLS_CACERTFILE, config.caFilePath.toStdString().c_str());
    }

    result = ldap_initialize(&ldap, config.ldapUrl.toStdString().c_str());
    if (result != LDAP_SUCCESS) {
        qCCritical(lcLDAPInitializer)
                << "Could not initialize LDAP handle from uri:" << ldap_err2string(result);

        if (ldap) {
            freeLDAPHandle(ldap);
        }
        return nullptr;
    }

    // Use LDAP version 3
    int version = 3;
    ldap_set_option(ldap, LDAP_OPT_PROTOCOL_VERSION, &version);

    // Use TLS
    if (config.useSSL) {
        result = ldap_start_tls_s(ldap, NULL, NULL);

        if (result != LDAP_SUCCESS) {
            qCCritical(lcLDAPInitializer)
                    << "Could not start TLS for LDAP:" << ldap_err2string(result);
            if (ldap) {
                freeLDAPHandle(ldap);
            }
            return nullptr;
        }

        qCDebug(lcLDAPInitializer) << "TLS installed on LDAP handle:" << ldap_tls_inplace(ldap);
    }

    // LDAP bind
    if (config.bindMethod != LDAPInitializer::BindMethod::None) {
        if (config.bindMethod == LDAPInitializer::BindMethod::Simple) {
            berval cred;
            const std::string pw = config.bindPassword.toStdString();
            cred.bv_val = (char *)pw.c_str();
            cred.bv_len = config.bindPassword.length();

            qCDebug(lcLDAPInitializer)
                    << "LDAP simple bind vs." << config.ldapUrl << "with" << config.bindDn;

            result = ldap_sasl_bind_s(ldap, config.bindDn.toStdString().c_str(), LDAP_SASL_SIMPLE,
                                      &cred, NULL, NULL, NULL);

        } else if (config.bindMethod == LDAPInitializer::BindMethod::GSSAPI) {
            result = ldap_sasl_interactive_bind_s(ldap, config.bindDn.toStdString().c_str(),
                                                  "GSSAPI", NULL, NULL, LDAP_SASL_INTERACTIVE,
                                                  &interact,
                                                  const_cast<LDAPInitializer::Config *>(&config));
        }

        if (result != LDAP_SUCCESS) {
            qCCritical(lcLDAPInitializer) << "Error on LDAP bind:" << ldap_err2string(result);
            if (ldap) {
                freeLDAPHandle(ldap);
            }
            return nullptr;
        }

        qCDebug(lcLDAPInitializer) << "LDAP bind successful";
    }

    return ldap;
}

void LDAPInitializer::freeLDAPHandle(LDAP *ldap)
{
    if (ldap) {
        ldap_unbind_ext_s(ldap, NULL, NULL);
    }
}
