#ifdef WIN32
#  include <winsock2.h>
#endif

#include <ldap.h>
#include <QDebug>
#include <QLoggingCategory>
#include <QSet>
#include <QThread>

#include "LDAPAddressBookFeeder.h"
#include "AddressBook.h"
#include "AddressBookManager.h"
#include "ErrorBus.h"
#include "ReadOnlyConfdSettings.h"
#include "AvatarManager.h"
#include "AddressBookManager.h"

Q_LOGGING_CATEGORY(lcLDAPAddressBookFeeder, "gonnect.app.feeder.LDAPAddressBookFeeder")

LDAPAddressBookFeeder::LDAPAddressBookFeeder(const QString &group, const int retryCount,
                                             const int retryInterval, AddressBookManager *parent)
    : QObject(parent), m_group(group), m_retryCount(retryCount), m_retryInterval(retryInterval)
{
    m_manager = qobject_cast<AddressBookManager *>(parent);

    connect(this, &LDAPAddressBookFeeder::newContactReady, this,
            [this](const QString &dn, const QString &sourceUid,
                   const Contact::ContactSourceInfo &contactSourceInfo, const QString &name,
                   const QString &company, const QString &mail, const QDateTime &lastModified,
                   const QList<Contact::PhoneNumber> &phoneNumbers, QPrivateSignal) {
                AddressBook::instance().addContact(dn, sourceUid, contactSourceInfo, name, company,
                                                   mail, lastModified, phoneNumbers, m_blockInfo);
            });

    connect(this, &LDAPAddressBookFeeder::newExternalImageAdded, this,
            [](const QString &id, const QByteArray &data, const QDateTime &modified,
               QPrivateSignal) { AvatarManager::instance().addExternalImage(id, data, modified); });
}

void LDAPAddressBookFeeder::init(const LDAPInitializer::Config &ldapConfig,
                                 QStringList sipStatusSubscriptableAttributes,
                                 const QString &baseNumber)
{
    m_ldapConfig = ldapConfig;
    m_baseNumber = baseNumber;
    m_sipStatusSubscriptableAttributes = sipStatusSubscriptableAttributes;
}

void LDAPAddressBookFeeder::resetFeeder()
{
    m_isProcessing = false;
    m_ldapConfig = {};

    if (m_ldap) {
        LDAPInitializer::freeLDAPHandle(m_ldap);
        m_ldap = nullptr;
    }

    m_baseNumber = "";
    m_sipStatusSubscriptableAttributes.clear();
    m_blockInfo = {};
    m_attrs = {};
}

void LDAPAddressBookFeeder::process()
{
    connect(
            this, &LDAPAddressBookFeeder::feederFailed, this,
            [this]() {
                // Prepare feeder for re-run
                resetFeeder();
                AddressBook::instance().removeContactsBySource(m_group);

                if (m_authFailed) {
                    // Previous run failed due to auth, we'll prompt the user again immediately
                    qCCritical(lcLDAPAddressBookFeeder)
                            << "Failed to process LDAP sources - invalid password";

                    process();
                } else {
                    // Some other error has occurred, wait and try again
                    if (m_retryCount > 0) {
                        m_retryCount--;

                        qCCritical(lcLDAPAddressBookFeeder)
                                << "Failed to process LDAP sources - trying later";

                        QTimer::singleShot(m_retryInterval, this, [this]() { process(); });
                    }
                }
            },
            Qt::SingleShotConnection);

    ReadOnlyConfdSettings settings;
    settings.beginGroup(m_group);

    m_displayName = settings.value("displayName", m_group).toString();
    bool ok = true;
    m_priority = settings.value("prio", 0).toUInt(&ok);
    if (!ok) {
        qCWarning(lcLDAPAddressBookFeeder) << "Could not parse priority value for" << m_group;
        m_priority = 0;
    }

    m_blockInfo.isBlocking = settings.value("block", false).toBool();
    m_blockInfo.responseCode =
            settings.value("blockSipCode", GONNECT_DEFAULT_BLOCK_SIP_CODE).toUInt();

    const auto bindMethodStr = settings.value("bindMethod", "none").toString();

    if (bindMethodStr == "simple" || bindMethodStr == "gssapi") {
        m_manager->acquireSecret(m_authFailed, m_group, [this](const QString &password) {
            m_authFailed = false;

            processImpl(password);
        });
    } else { // "none"
        processImpl("");
    }
}

void LDAPAddressBookFeeder::processImpl(const QString &password)
{
    ReadOnlyConfdSettings settings;

    QString url = settings.value(m_group + "/url", "").toString();

    settings.beginGroup(m_group);
    const auto subscribableAttributes =
            settings.value("sipStatusSubscriptableAttributes", "").toStringList();

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
        // INFO: This would point towards a config issue - not going to retry automatically
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

    m_attrs.name = settings.value("attrName", "cn").toByteArray();
    m_attrs.uid = settings.value("attrUid", "uid").toByteArray();
    m_attrs.company = settings.value("attrCompany", "o").toByteArray();
    m_attrs.email = settings.value("attrEmail", "mail").toByteArray();
    m_attrs.commercial = settings.value("attrCommercial", "telephoneNumber").toByteArray();
    m_attrs.mobile = settings.value("attrMobile", "mobile").toByteArray();
    m_attrs.home = settings.value("attrHome", "homePhone").toByteArray();
    m_attrs.avatar = settings.value("attrAvatar", "jpegPhoto").toByteArray();
    m_pageSize = settings.value("pageSize", 500).toInt();

    init(ldapConfig, subscribableAttributes, settings.value("baseNumber", "").toString());

    m_isProcessing = true;

    feedAddressBook();

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

char **LDAPAddressBookFeeder::toCStringList(const QList<QByteArray> &values) const
{
    size_t i = 0;
    char **arr = (char **)malloc((values.size() + 1) * sizeof(char *));

    for (const QByteArray &value : std::as_const(values)) {
        const size_t sz = value.size() + 1;
        char *p = (char *)malloc(sz);
        strncpy(p, value.constData(), sz);
        arr[i++] = p;
    }

    arr[i] = nullptr;

    return arr;
}

bool LDAPAddressBookFeeder::pagedSearch(
        LDAP *ldap, const LDAPInitializer::Config &ldapConfig, char **attrs,
        const std::function<void(LDAP *, LDAPMessage *)> &onEntry) const
{
    const QByteArray base = ldapConfig.ldapBase.toLocal8Bit();
    const QByteArray filter = ldapConfig.ldapFilter.toUtf8();
    const ber_int_t pageSize = m_pageSize > 0 ? m_pageSize : 500;

    struct berval cookie = { 0, nullptr };
    int totalEntries = 0;
    bool ok = true;

    while (true) {
        LDAPControl *pageControl = nullptr;

        // Create page control with isCritical=0 to have it working independently of an
        // LDAP server supporting it or not.
        int rc = ldap_create_page_control(ldap, pageSize, &cookie, 0, &pageControl);
        if (rc != LDAP_SUCCESS) {
            qCCritical(lcLDAPAddressBookFeeder)
                    << "Could not create paged results control:" << ldap_err2string(rc);
            ErrorBus::instance().addError(tr("LDAP error: %1").arg(ldap_err2string(rc)));
            ok = false;
            break;
        }

        LDAPControl *serverControls[2] = { pageControl, nullptr };
        LDAPMessage *result = nullptr;

        rc = ldap_search_ext_s(ldap, base.constData(), LDAP_SCOPE_SUBTREE, filter.constData(),
                               attrs, 0, serverControls, nullptr, nullptr, LDAP_NO_LIMIT, &result);

        ldap_control_free(pageControl);

        if (rc != LDAP_SUCCESS && rc != LDAP_SIZELIMIT_EXCEEDED && rc != LDAP_ADMINLIMIT_EXCEEDED) {
            qCCritical(lcLDAPAddressBookFeeder)
                    << "paged search request failed:" << ldap_err2string(rc);
            ErrorBus::instance().addError(tr("LDAP error: %1").arg(ldap_err2string(rc)));
            if (result) {
                ldap_msgfree(result);
            }
            ok = false;
            break;
        }

        for (LDAPMessage *entry = ldap_first_entry(ldap, result); entry != nullptr;
             entry = ldap_next_entry(ldap, entry)) {
            onEntry(ldap, entry);
            ++totalEntries;
        }

        LDAPControl **returnedControls = nullptr;
        const int parseRc = ldap_parse_result(ldap, result, nullptr, nullptr, nullptr, nullptr,
                                              &returnedControls, 0);
        ldap_msgfree(result);

        if (parseRc != LDAP_SUCCESS) {
            qCCritical(lcLDAPAddressBookFeeder)
                    << "failed to parse paged search result:" << ldap_err2string(parseRc);
            ErrorBus::instance().addError(tr("LDAP error: %1").arg(ldap_err2string(parseRc)));
            if (returnedControls) {
                ldap_controls_free(returnedControls);
            }
            ok = false;
            break;
        }

        if (cookie.bv_val) {
            ber_memfree(cookie.bv_val);
            cookie.bv_val = nullptr;
            cookie.bv_len = 0;
        }

        // Check for more pages
        bool morePages = false;
        if (returnedControls) {
            if (LDAPControl *pageResponse =
                        ldap_control_find(LDAP_CONTROL_PAGEDRESULTS, returnedControls, nullptr)) {
                ber_int_t estimatedTotal = 0;
                ldap_parse_pageresponse_control(ldap, pageResponse, &estimatedTotal, &cookie);
                morePages = cookie.bv_val != nullptr && cookie.bv_len > 0;
            }
            ldap_controls_free(returnedControls);
        }

        if (!morePages) {
            break;
        }
    }

    if (cookie.bv_val) {
        ber_memfree(cookie.bv_val);
    }

    qCInfo(lcLDAPAddressBookFeeder) << totalEntries << "search entries retrieved";
    return ok;
}

void LDAPAddressBookFeeder::feedAddressBook()
{
    int result = 0;

    m_ldap = LDAPInitializer::initialize(m_ldapConfig, result);
    if (!m_ldap) {
        qCCritical(lcLDAPAddressBookFeeder)
                << "Could not get LDAP connection:" << ldap_err2string(result);
        ErrorBus::instance().addError(
                tr("Failed to initialize LDAP connection: %1").arg(ldap_err2string(result)));

        if (result == LDAP_INVALID_CREDENTIALS) {
            m_authFailed = true;
        }
        Q_EMIT feederFailed();
        return;
    }

    startContactQuery();
}

void LDAPAddressBookFeeder::loadAvatars(const QList<const Contact *> &contacts)
{
    if (m_attrs.name.isEmpty() || m_attrs.avatar.isEmpty()) {
        return;
    }

    const auto nameAttr = QString::fromLatin1(m_attrs.name);
    QStringList filterList;
    filterList.reserve(contacts.size());

    for (const Contact *contact : contacts) {
        filterList.append(QString("(%1=%2)").arg(nameAttr, contact->name()));
    }

    LDAPInitializer::Config newConfig(m_ldapConfig);
    newConfig.ldapFilter =
            QString("(& %1 (| %2))").arg(m_ldapConfig.ldapFilter, filterList.join(' '));

    loadAllAvatars(newConfig);
}

void LDAPAddressBookFeeder::loadAllAvatars(const LDAPInitializer::Config &ldapConfig)
{
    const QByteArray avatarAttr = m_attrs.avatar;
    m_isProcessing = true;

    QThread::create([this, ldapConfig, avatarAttr]() {
        QList<QByteArray> attributes = { QByteArrayLiteral("modifyTimestamp") };
        if (!avatarAttr.isEmpty()) {
            attributes.append(avatarAttr);
        }

        char **attrs = toCStringList(attributes);

        qCInfo(lcLDAPAddressBookFeeder) << "Connecting to LDAP service" << ldapConfig.ldapUrl;

        int result = 0;
        LDAP *ldap = LDAPInitializer::initialize(ldapConfig, result);
        if (!ldap) {
            qCCritical(lcLDAPAddressBookFeeder)
                    << "Could not initialize LDAP handle from uri:" << ldap_err2string(result);
            ErrorBus::instance().addError(
                    tr("Failed to initialize LDAP connection: %1").arg(ldap_err2string(result)));
            clearCStringlist(attrs);
            m_isProcessing = false;

            if (result == LDAP_INVALID_CREDENTIALS) {
                m_authFailed = true;
            }
            Q_EMIT feederFailed();
            return;
        }

        const bool ok = pagedSearch(ldap, ldapConfig, attrs,
                                    [this, &avatarAttr](LDAP *ld, LDAPMessage *entry) {
                                        parseAvatarEntry(ld, entry, avatarAttr);
                                    });

        clearCStringlist(attrs);
        LDAPInitializer::freeLDAPHandle(ldap);
        m_isProcessing = false;

        if (!ok) {
            Q_EMIT feederFailed();
        }
    })->start();
}

QUrl LDAPAddressBookFeeder::networkCheckURL() const
{
    ReadOnlyConfdSettings settings;

    QUrl url(settings.value(m_group + "/url", "").toString());
    if (url.port() == -1) {
        url.setPort(389); // default port
    }
    return url;
}

void LDAPAddressBookFeeder::startContactQuery()
{
    QThread::create([this]() {
        QSet<QByteArray> requested = { QByteArrayLiteral("modifyTimestamp") };
        for (const QByteArray *role :
             { &m_attrs.name, &m_attrs.uid, &m_attrs.company, &m_attrs.email, &m_attrs.commercial,
               &m_attrs.mobile, &m_attrs.home }) {
            if (!role->isEmpty()) {
                requested.insert(*role);
            }
        }
        char **attrs = toCStringList(QList<QByteArray>(requested.cbegin(), requested.cend()));

        const bool ok =
                pagedSearch(m_ldap, m_ldapConfig, attrs,
                            [this](LDAP *ld, LDAPMessage *entry) { parseContactEntry(ld, entry); });

        clearCStringlist(attrs);
        LDAPInitializer::freeLDAPHandle(m_ldap);
        m_ldap = nullptr;

        if (!ok) {
            Q_EMIT feederFailed();
            return;
        }

        QMetaObject::invokeMethod(
                this, []() { Q_EMIT AddressBook::instance().contactsReady(); },
                Qt::QueuedConnection);

        QMetaObject::invokeMethod(
                this, [this]() { loadAvatarsForContacts(); }, Qt::QueuedConnection);
    })->start();
}

void LDAPAddressBookFeeder::parseContactEntry(LDAP *ldap, LDAPMessage *entry)
{
    auto stripBaseNumber = [this](QString num) {
        if (num.startsWith(m_baseNumber)) {
            num = num.sliced(m_baseNumber.size());
        }
        return num;
    };

    QString dn, cn, sourceUid, company, mail, modifyTimestamp;
    QList<Contact::PhoneNumber> phoneNumbers;

    if (char *dnTemp = ldap_get_dn(ldap, entry)) {
        dn = dnTemp;
        ldap_memfree(dnTemp);
    }

    BerElement *ber = nullptr;
    for (char *a = ldap_first_attribute(ldap, entry, &ber); a != nullptr;
         a = ldap_next_attribute(ldap, entry, ber)) {

        if (struct berval **vals = ldap_get_values_len(ldap, entry, a)) {
            const auto val = (**vals).bv_val;

            auto matches = [a](const QByteArray &name) {
                return !name.isEmpty() && qstricmp(a, name.constData()) == 0;
            };

            if (matches(m_attrs.name)) {
                cn = val;
            }
            if (matches(m_attrs.company)) {
                company = val;
            }
            if (matches(m_attrs.uid)) {
                sourceUid = val;
            }
            if (matches(m_attrs.email)) {
                mail = val;
            }
            if (qstricmp(a, "modifyTimestamp") == 0) {
                modifyTimestamp = val;
            }
            if (matches(m_attrs.commercial)) {
                phoneNumbers.append({ Contact::NumberType::Commercial, stripBaseNumber(val),
                                      m_sipStatusSubscriptableAttributes.contains(
                                              QString::fromLatin1(m_attrs.commercial)) });
            }
            if (matches(m_attrs.mobile)) {
                phoneNumbers.append({ Contact::NumberType::Mobile, stripBaseNumber(val),
                                      m_sipStatusSubscriptableAttributes.contains(
                                              QString::fromLatin1(m_attrs.mobile)) });
            }
            if (matches(m_attrs.home)) {
                phoneNumbers.append({ Contact::NumberType::Home, stripBaseNumber(val),
                                      m_sipStatusSubscriptableAttributes.contains(
                                              QString::fromLatin1(m_attrs.home)) });
            }

            ldap_value_free_len(vals);
        }

        ldap_memfree(a);
    }

    if (ber) {
        ber_free(ber, 0);
    }

    Q_EMIT newContactReady(dn, sourceUid, { m_priority, m_displayName, m_group }, cn, company, mail,
                           QDateTime::fromString(modifyTimestamp, "yyyyMMddhhmmsst"), phoneNumbers,
                           QPrivateSignal());
}

void LDAPAddressBookFeeder::parseAvatarEntry(LDAP *ldap, LDAPMessage *entry,
                                             const QByteArray &avatarAttr)
{
    QString dn;
    QDateTime modifyTimestamp;
    QByteArray jpegPhoto;

    if (char *dnTemp = ldap_get_dn(ldap, entry)) {
        dn = dnTemp;
        ldap_memfree(dnTemp);
    }

    BerElement *ber = nullptr;
    for (char *a = ldap_first_attribute(ldap, entry, &ber); a != nullptr;
         a = ldap_next_attribute(ldap, entry, ber)) {

        if (struct berval **vals = ldap_get_values_len(ldap, entry, a)) {
            if (!avatarAttr.isEmpty() && qstricmp(a, avatarAttr.constData()) == 0) {
                jpegPhoto = QByteArray((**vals).bv_val, static_cast<qsizetype>((**vals).bv_len));
            } else if (qstricmp(a, "modifyTimestamp") == 0) {
                modifyTimestamp = QDateTime::fromString(QString::fromUtf8((**vals).bv_val),
                                                        "yyyyMMddhhmmsst");
            }

            ldap_value_free_len(vals);
        }

        ldap_memfree(a);
    }

    if (ber) {
        ber_free(ber, 0);
    }

    if (!jpegPhoto.isEmpty()) {
        const auto contactId = AddressBook::instance().hashifyCn(dn);
        Q_EMIT newExternalImageAdded(contactId, jpegPhoto, modifyTimestamp, QPrivateSignal());
    }
}

void LDAPAddressBookFeeder::loadAvatarsForContacts()
{
    // Not configured - mark us as ready
    if (m_attrs.name.isEmpty() || m_attrs.avatar.isEmpty()) {
        m_isProcessing = false;
        return;
    }

    bool firstRun = false;
    const auto dirtyContacts = AvatarManager::instance().initialLoad(&firstRun);

    if (firstRun) {
        loadAllAvatars(m_ldapConfig);
    } else if (!dirtyContacts.isEmpty()) {
        loadAvatars(dirtyContacts);
    } else {
        m_isProcessing = false;
    }
}
