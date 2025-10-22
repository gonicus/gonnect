#pragma once

#include <QObject>
#include <ldap.h>

/*!
 * \brief The LDAPInitializer static class initializes an LDAP handle
 *
 * Given a config object, the method initialize() tries to connect to LDAP and create an LDAP
 * handle. It must be freed with freeLDAPHandle() after use.
 */
class LDAPInitializer
{
public:
    enum class BindMethod { None, Simple, GSSAPI };

    struct Config
    {
        bool useSSL = false;
        BindMethod bindMethod = BindMethod::None;

        QString caFilePath;
        QString ldapUrl;
        QString ldapBase;
        QString ldapFilter;
        QString bindDn;
        QString bindPassword;
        QString saslRealm;
        QString saslAuthcid;
        QString saslAuthzid;
    };

    explicit LDAPInitializer() = delete;

    /*!
     * @brief initialize Try to create a new LDAP connection and handle with
     * @param config
     * @return Pointer to an ldap handle (must be freed with freeLDAPHandle after use) or a nullptr
     * if something did not work
     */
    [[nodiscard]] static LDAP *initialize(const Config &config);

    /*!
     * \brief freeLDAPHandle Destroys and frees an LDAP handle
     * \param ldap
     */
    static void freeLDAPHandle(LDAP *ldap);
};
