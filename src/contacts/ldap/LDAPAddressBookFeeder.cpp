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

    m_ldapSearchMessageId = -1;
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

    init(ldapConfig,
         scriptableAttributes.isEmpty() ? QStringList() : scriptableAttributes.split(QChar(',')),
         settings.value("baseNumber", "").toString());

    feedAddressBook();

    const auto dirtyContacts = AvatarManager::instance().initialLoad();
    if (dirtyContacts.isEmpty()) {
        loadAllAvatars(m_ldapConfig);
    } else {
        loadAvatars(dirtyContacts);
    }

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
    QSet<QByteArray> requested = { QByteArrayLiteral("modifyTimestamp") };
    for (const QByteArray *role : { &m_attrs.name, &m_attrs.uid, &m_attrs.company, &m_attrs.email,
                                    &m_attrs.commercial, &m_attrs.mobile, &m_attrs.home }) {
        if (!role->isEmpty()) {
            requested.insert(*role);
        }
    }

    int result = 0;
    size_t i = 0;
    char **attrs = (char **)malloc((requested.size() + 1) * sizeof(char *));
    for (const QByteArray &attr : std::as_const(requested)) {
        size_t sz = attr.size() + 1;
        char *p = (char *)malloc(sz);
        strncpy(p, attr.constData(), sz);
        attrs[i++] = p;
    }
    attrs[i] = NULL;

    m_ldap = LDAPInitializer::initialize(m_ldapConfig, result);
    if (!m_ldap) {
        qCCritical(lcLDAPAddressBookFeeder)
                << "Could not get LDAP connection:" << ldap_err2string(result);
        ErrorBus::instance().addError(
                tr("Failed to initialize LDAP connection: %1").arg(ldap_err2string(result)));
        clearCStringlist(attrs);

        if (result == LDAP_INVALID_CREDENTIALS) {
            m_authFailed = true;
        }
        Q_EMIT feederFailed();
        return;
    }

    timeval timeout;
    timeout.tv_sec = 3;
    timeout.tv_usec = 0;

    result = ldap_search_ext(m_ldap, m_ldapConfig.ldapBase.toLocal8Bit().data(), LDAP_SCOPE_SUBTREE,
                             m_ldapConfig.ldapFilter.toStdString().c_str(), attrs, false, NULL,
                             NULL, &timeout, 0, &m_ldapSearchMessageId);

    clearCStringlist(attrs);

    if (result == LDAP_SUCCESS) {
        startContactQuery();
    } else {
        qCCritical(lcLDAPAddressBookFeeder)
                << "Error on search request: " << ldap_err2string(result);
        ErrorBus::instance().addError(tr("LDAP error: %1").arg(ldap_err2string(result)));

        Q_EMIT feederFailed();
        return;
    }
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
        char *a = nullptr;
        char *dnTemp = nullptr;
        BerElement *ber = nullptr;
        struct berval **vals;
        char *matchedMsg = nullptr;
        char *errorMsg = nullptr;
        int numEntries = 0, numRefs = 0, result = 0, msgType = 0, parseResultCode = 0;
        LDAP *ldap = nullptr;
        LDAPMessage *msg = nullptr;

        QList<QByteArray> attributes = { QByteArrayLiteral("modifyTimestamp") };
        if (!avatarAttr.isEmpty()) {
            attributes.append(avatarAttr);
        }

        size_t i = 0;
        char **attrs = (char **)malloc((attributes.count() + 1) * sizeof(char *));
        for (const QByteArray &attr : std::as_const(attributes)) {
            size_t sz = attr.size() + 1;
            char *p = (char *)malloc(sz);
            strncpy(p, attr.constData(), sz);
            attrs[i++] = p;
        }
        attrs[i] = NULL;

        QString dn;
        QDateTime modifyTimestamp;
        QByteArray jpegPhoto;
        int count = 0;

        qCInfo(lcLDAPAddressBookFeeder) << "Connecting to LDAP service" << ldapConfig.ldapUrl;

        ldap = LDAPInitializer::initialize(ldapConfig, result);
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

        result = ldap_search_ext_s(ldap, ldapConfig.ldapBase.toLocal8Bit().data(),
                                   LDAP_SCOPE_SUBTREE, ldapConfig.ldapFilter.toStdString().c_str(),
                                   attrs, false, NULL, NULL, NULL, LDAP_NO_LIMIT, &msg);

        clearCStringlist(attrs);

        if (result != LDAP_SUCCESS) {
            qCCritical(lcLDAPAddressBookFeeder)
                    << "Error on search request: " << ldap_err2string(result);
            ErrorBus::instance().addError(tr("LDAP error: %1").arg(ldap_err2string(result)));

            m_isProcessing = false;
            Q_EMIT feederFailed();
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
                modifyTimestamp = QDateTime();
                jpegPhoto = "";

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

                        char *val = (**vals).bv_val;

                        if (!avatarAttr.isEmpty() && qstricmp(a, avatarAttr.constData()) == 0) {
                            for (uint i = 0; i < (**vals).bv_len; ++i) {
                                jpegPhoto.append(*val);
                                val += sizeof(char);
                            }
                        } else if (qstricmp(a, "modifyTimestamp") == 0) {
                            modifyTimestamp = QDateTime::fromString(val, "yyyyMMddhhmmsst");
                        }

                        ldap_value_free_len(vals);
                    }

                    ldap_memfree(a);
                }

                if (!jpegPhoto.isEmpty()) {
                    const auto contactId = AddressBook::instance().hashifyCn(dn);
                    Q_EMIT newExternalImageAdded(contactId, jpegPhoto, modifyTimestamp,
                                                 QPrivateSignal());

                    ++count;
                }

                a = nullptr;

                ber_free(ber, 0);
                ber = nullptr;

                break;
            }

            case LDAP_RES_SEARCH_RESULT: {
                parseResultCode = ldap_parse_result(ldap, msg, &result, &matchedMsg, &errorMsg,
                                                    NULL, NULL, 1);
                if (parseResultCode != LDAP_SUCCESS) {
                    qCCritical(lcLDAPAddressBookFeeder)
                            << "LDAP parse error:" << ldap_err2string(parseResultCode);
                    ErrorBus::instance().addError(
                            tr("Parse error: %1").arg(ldap_err2string(parseResultCode)));

                    m_isProcessing = false;
                    Q_EMIT feederFailed();
                    return;
                }
                break;
            }

            default:
                qCCritical(lcLDAPAddressBookFeeder) << "Unknown message type:" << msgType;

                m_isProcessing = false;
                Q_EMIT feederFailed();
                return;
            }
        }

        qCInfo(lcLDAPAddressBookFeeder) << "Loaded" << count << "avatars";

        LDAPInitializer::freeLDAPHandle(ldap);
        m_isProcessing = false;
    })->start();
}

QUrl LDAPAddressBookFeeder::networkCheckURL() const
{
    ReadOnlyConfdSettings settings;

    QString url = settings.value(m_group + "/url", "").toString();
    return QUrl(url);
}

void LDAPAddressBookFeeder::startContactQuery()
{
    QThread::create([this]() {
        timeval timeout;
        timeout.tv_sec = 3;
        timeout.tv_usec = 0;

        LDAPMessage *msg = nullptr;

        const int ldapResult = ldap_result(m_ldap, m_ldapSearchMessageId, true, &timeout, &msg);

        if (ldapResult > 0) { // Success
            processResult(msg);
        } else if (ldapResult == 0) { // Timeout
            qCCritical(lcLDAPAddressBookFeeder)
                    << "Timeout on search request: " << ldap_err2string(ldapResult);
            ErrorBus::instance().addError(tr("LDAP timeout: %1").arg(ldap_err2string(ldapResult)));
        } else { // Error
            qCCritical(lcLDAPAddressBookFeeder)
                    << "Error on search request: " << ldap_err2string(ldapResult);
            ErrorBus::instance().addError(tr("LDAP error: %1").arg(ldap_err2string(ldapResult)));
        }
    })->start();
}

void LDAPAddressBookFeeder::processResult(LDAPMessage *ldapMessage)
{
    char *a = nullptr;
    char *dnTemp = nullptr;
    BerElement *ber = nullptr;
    struct berval **vals;
    char *matchedMsg = nullptr;
    char *errorMsg = nullptr;
    int numEntries = 0, numRefs = 0, result = 0, msgType = 0, parseResultCode = 0;
    LDAPMessage *msg = ldapMessage;
    QString dn, cn, sourceUid, company, mail, modifyTimestamp;
    QList<Contact::PhoneNumber> phoneNumbers;

    auto stripBaseNumber = [this](QString num) {
        if (num.startsWith(m_baseNumber)) {
            num = num.sliced(m_baseNumber.size());
        }
        return num;
    };

    numEntries = ldap_count_entries(m_ldap, msg);
    numRefs = ldap_count_references(m_ldap, msg);

    qCInfo(lcLDAPAddressBookFeeder)
            << "Retrieved" << numEntries << "entries and" << numRefs << "refs";

    for (msg = ldap_first_message(m_ldap, msg); msg != NULL; msg = ldap_next_message(m_ldap, msg)) {
        msgType = ldap_msgtype(msg);

        switch (msgType) {
        case LDAP_RES_SEARCH_ENTRY: {
            dn = "";
            cn = "";
            company = "";
            mail = "";
            modifyTimestamp = "";
            phoneNumbers.clear();

            // Iterate over attributes
            for (a = ldap_first_attribute(m_ldap, msg, &ber); a != NULL;
                 a = ldap_next_attribute(m_ldap, msg, ber)) {

                // Get DN
                if ((dnTemp = ldap_get_dn(m_ldap, msg)) != NULL) {
                    dn = dnTemp;
                    ldap_memfree(dnTemp);
                }

                // Iterate over values
                if ((vals = ldap_get_values_len(m_ldap, msg, a)) != NULL) {
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
            }

            Q_EMIT newContactReady(dn, sourceUid, { m_priority, m_displayName, m_group }, cn,
                                   company, mail,
                                   QDateTime::fromString(modifyTimestamp, "yyyyMMddhhmmsst"),
                                   phoneNumbers, QPrivateSignal());

            a = nullptr;

            ber_free(ber, 0);
            ber = nullptr;

            break;
        }

        case LDAP_RES_SEARCH_RESULT: {
            parseResultCode =
                    ldap_parse_result(m_ldap, msg, &result, &matchedMsg, &errorMsg, NULL, NULL, 1);
            if (parseResultCode != LDAP_SUCCESS) {
                qCCritical(lcLDAPAddressBookFeeder)
                        << "LDAP parse error:" << ldap_err2string(parseResultCode);
                ErrorBus::instance().addError(
                        tr("Parse error: %1").arg(ldap_err2string(parseResultCode)));

                Q_EMIT feederFailed();
                return;
            }
            break;
        }

        default:
            qCCritical(lcLDAPAddressBookFeeder) << "Unknown message type:" << msgType;

            Q_EMIT feederFailed();
            return;
        }
    }

    LDAPInitializer::freeLDAPHandle(m_ldap);
    m_ldap = nullptr;
}
