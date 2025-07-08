#include <QRegularExpression>
#include <QLoggingCategory>
#include <QJsonDocument>
#include <QJsonObject>
#include "ReadOnlyConfdSettings.h"
#include "PhoneNumberUtil.h"
#include "PhoneCodeLookup.h"
#include "AddressBook.h"

#ifndef APP_TESTS
#  include "AvatarManager.h"
#endif

Q_LOGGING_CATEGORY(lcPhoneNumberUtil, "gonnect.app.contacts.PhoneNumberUtil")

QString ContactInfo::toString() const
{
    QJsonObject request{ { "$schema", "request.schema.json" },
                         { "sipURI", sipUrl },
                         { "name", contact ? contact->name() : displayName },
                         { "company", contact ? contact->company() : "" },
                         { "email", contact ? contact->mail() : "" } };

    QJsonDocument doc(request);
    return doc.toJson(QJsonDocument::JsonFormat::Compact);
}

QString PhoneNumberUtil::cleanPhoneNumber(const QString &number)
{
    QString result(number);

    static const QRegularExpression stripRegEx("[^0-9#+*]");

    result.replace(stripRegEx, "");

    if (!result.size()) {
        return result;
    }

    if (result.size() <= 3) {
        return result;
    }

    static const QRegularExpression doubleZeroAtBeginning("^00");
    result.replace(doubleZeroAtBeginning, "+");

    static const QRegularExpression singleZeroAtBeginning("^0");
    result.replace(singleZeroAtBeginning, "+49");

    if (result.at(0) != '+') {
        result.prepend('+');
    }

    return result;
}

QDebug operator<<(QDebug debug, const ContactInfo &contactInfo)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "ContactInfo("
                    << "SIP url: " << contactInfo.sipUrl << ", "
                    << "Phone number: " << contactInfo.phoneNumber << ", "
                    << "City: " << contactInfo.city << ", "
                    << "Countries: " << contactInfo.countries.join(", ");

    if (contactInfo.contact) {
        debug.nospace() << ", Contact: " << *(contactInfo.contact);
    }
    debug.nospace() << ")";

    return debug;
}

ContactInfo PhoneNumberUtil::contactInfoBySipUrl(const QString &sipUrl)
{

    if (m_contactInfoCache.contains(sipUrl)) {
        return m_contactInfoCache.value(sipUrl);
    }

    ReadOnlyConfdSettings settings;

    // Extract phone number from sip url
    auto phoneNumber = sipUrl;
    static const QRegularExpression sipNumberRegex("^.*sips?:(.*)@.*$",
                                                   QRegularExpression::CaseInsensitiveOption);
    phoneNumber.replace(sipNumberRegex, "\\1");

    static const QRegularExpression international("^000(.*)$");
    phoneNumber.replace(international, "+\\1");

    static const QRegularExpression national("^00(.*)$");
    QString nationalPrefix = settings.value("generic/nationalPrefix").toString();
    if (!nationalPrefix.isEmpty()) {
        phoneNumber.replace(national, nationalPrefix + "\\1");
    }

    static const QRegularExpression regional("^0(.*)$");
    QString regionalPrefix = settings.value("generic/regionalPrefix").toString();
    if (!regionalPrefix.isEmpty()) {
        phoneNumber.replace(regional, regionalPrefix + "\\1");
    }

    ContactInfo info;
    info.sipUrl = sipUrl;
    info.phoneNumber = phoneNumber;
    info.contact = AddressBook::instance().lookupByNumber(phoneNumber);
    info.countries = PhoneCodeLookup::countryNameFromPhoneNumber(phoneNumber);
    info.city = PhoneCodeLookup::cityNameFromPhoneNumber(phoneNumber);
    info.isAnonymous = isNumberAnonymous(sipUrl);

    // Display name
    if (info.isAnonymous) {
        info.displayName = tr("Anonymous");
    } else if (info.contact && !info.contact->name().isEmpty()) {
        info.displayName = info.contact->name();
    } else {
        // Fallback 1: Use string from sip url
        static const QRegularExpression nameFromUrlExp("\"(.*)\"");
        const auto matches = nameFromUrlExp.match(sipUrl);
        if (matches.hasMatch()) {
            info.displayName = matches.captured(1);
        }
    }
    if (info.displayName.isEmpty()) {
        // Fallback 2: Use phone number
        info.displayName = info.phoneNumber;
    }

    if (info.contact) {
        const auto numbers = info.contact->phoneNumbers();
        for (const auto &num : numbers) {
            if (num.number == phoneNumber) {
                info.numberType = num.type;
                info.isSipSubscriptable = num.isSipSubscriptable;
                break;
            }
        }
    }

    m_contactInfoCache.insert(sipUrl, info);

    return info;
}

bool PhoneNumberUtil::isSipUri(const QString &str)
{
    static const QRegularExpression sipNumberRegex("^.*sips?:[0-9a-zA-Z_+*#%-]+@.*$",
                                                   QRegularExpression::CaseInsensitiveOption);
    return sipNumberRegex.match(str).hasMatch();
}

QString PhoneNumberUtil::numberFromSipUrl(const QString &sipUrl)
{
    static const QRegularExpression sipNumberRegex("^.*sips?:(.*)@.*$",
                                                   QRegularExpression::CaseInsensitiveOption);

    const auto matchResult = sipNumberRegex.match(sipUrl);
    if (matchResult.hasMatch()) {
        return matchResult.captured(1);
    }
    return "";
}

bool PhoneNumberUtil::isEmergencyCallUrl(const QString &sipUrl)
{
    static bool isEmergencyRegexInitalized = false;
    static bool hasEmergencyRegex = false;
    static QRegularExpression regex;

    if (!isEmergencyRegexInitalized) {
        isEmergencyRegexInitalized = true;

        ReadOnlyConfdSettings settings;
        const QString regexString = settings.value("generic/emergencyRegex", "").toString();

        if (!regexString.isEmpty()) {
            regex = QRegularExpression(regexString);
            if (!regex.isValid()) {
                qCCritical(lcPhoneNumberUtil) << "Found regular expression string for emergency "
                                                 "sip url, but it is invalid and will be ignored:"
                                              << regex.errorString();
                return false;
            } else {
                hasEmergencyRegex = true;
            }
        }
    }

    if (hasEmergencyRegex) {
        return regex.match(sipUrl).hasMatch();
    }
    return false;
}

bool PhoneNumberUtil::isNumberAnonymous(const QString &sipUrl)
{
    static bool _initialized = false;
    static bool _hasValidRegex = false;
    static QRegularExpression anonymousRegex;

    if (!_initialized) {
        _initialized = true;

        ReadOnlyConfdSettings settings;
        const QString anonymousRegexString =
                settings.value("generic/anonymousRegex", "").toString();

        if (!anonymousRegexString.isEmpty()) {
            anonymousRegex.setPattern(anonymousRegexString);

            if (anonymousRegex.isValid()) {
                _hasValidRegex = true;
            } else {
                qCCritical(lcPhoneNumberUtil)
                        << "Found regular expression string for anonymous sip "
                           "url, but it is invalid and will be ignored:"
                        << anonymousRegex.errorString();
            }
        }
    }

    return _hasValidRegex ? anonymousRegex.match(sipUrl).hasMatch() : false;
}

PhoneNumberUtil::PhoneNumberUtil() : QObject()
{
    connect(&AddressBook::instance(), &AddressBook::contactsCleared, this,
            [this]() { m_contactInfoCache.clear(); });
    connect(&AddressBook::instance(), &AddressBook::contactsReady, this,
            [this]() { m_contactInfoCache.clear(); });

#ifndef APP_TESTS
    connect(&AvatarManager::instance(), &AvatarManager::avatarsLoaded, this,
            [this]() { m_contactInfoCache.clear(); });
#endif
}

QString PhoneNumberUtil::clearInternationalChars(const QString &str)
{
    auto s = str; // Ensure copy of str

    static QList<std::pair<const QRegularExpression, const QString>> replacements = {
        { QRegularExpression("��"), "ae" },
        { QRegularExpression("��"), "Ae" },
        { QRegularExpression("��|��"), "oe" },
        { QRegularExpression("��|��"), "Oe" },
        { QRegularExpression("��"), "ue" },
        { QRegularExpression("��"), "Ue" },
        { QRegularExpression("��"), "ss" },
        { QRegularExpression("���"), "Ss" },
        { QRegularExpression("��|��|��|��|��|��|��|��|��|��"), "a" },
        { QRegularExpression("��|��|��|��|��|��|��|��|��|��"), "A" },
        { QRegularExpression("��|��|��|��|��"), "c" },
        { QRegularExpression("��|��|��|��|��"), "C" },
        { QRegularExpression("��|��|��|��|��|��|��|��|��"), "e" },
        { QRegularExpression("��|��|��|��|��|��|��|��|��"), "E" },
        { QRegularExpression("��|��|��|��|��|��|��|��|��"), "i" },
        { QRegularExpression("��|��|��|��|��|��|��|��|I"), "I" },
        { QRegularExpression("��"), "j" },
        { QRegularExpression("��"), "J" },
        { QRegularExpression("��"), "l" },
        { QRegularExpression("��"), "L" },
        { QRegularExpression("��|��|��|�� "), "n" },
        { QRegularExpression("��|��|��|��"), "N" },
        { QRegularExpression("��|��|��|��|��|��|�� "), "o" },
        { QRegularExpression("��|��|��|��|��|��|��"), "O" },
        { QRegularExpression("��|��|��|�� "), "s" },
        { QRegularExpression("��|��|��|��"), "S" },
        { QRegularExpression("��|��|��|��|��|��|��|��|�� "), "u" },
        { QRegularExpression("��|��|��|��|��|��|��|��|��"), "U" },
        { QRegularExpression("��"), "w" },
        { QRegularExpression("��"), "W" },
        { QRegularExpression("��|��|��"), "y" },
        { QRegularExpression("��|��|��"), "Y" },
        { QRegularExpression("��|��|��"), "z" },
        { QRegularExpression("��|��|��"), "Z" },
    };

    for (const auto &pair : std::as_const(replacements)) {
        s = s.replace(pair.first, pair.second);
    }

    return s;
}
