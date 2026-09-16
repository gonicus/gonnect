#pragma once
#include <QList>
#include <QString>
#include <QStringList>
#include <QObject>

// Parser for the Remote-Party-ID header (draft-ietf-sip-privacy-03).
struct SIPRemotePartyId
{
    Q_GADGET

public:
    // Info about if we're calling or beeing called
    enum Party {
        Unknown,
        Calling,
        Called,
    };
    Q_ENUM(Party)

    // Privace restrictions for Remote-Party-ID information
    enum Privacy {
        Off,
        Name,
        Uri,
        Full,
    };
    Q_ENUM(Privacy)

    bool isNameRestricted() const
    {
        return m_privacy == Privacy::Name || m_privacy == Privacy::Full;
    }
    bool isNumberRestricted() const
    {
        return m_privacy == Privacy::Uri || m_privacy == Privacy::Full;
    }
    bool isValid() const { return m_valid; }

    QString uri() const { return m_uri; }
    QString name() const { return isNameRestricted() ? QString() : m_rawDisplayName; }
    QString number() const { return isNumberRestricted() ? QString() : m_rawNumber; }
    QString callbackNumber() const { return m_callbackNumber; }

    QString rawDisplayName() const { return m_rawDisplayName; }
    QString rawNumber() const { return m_rawNumber; }

    bool isScreened() const { return m_screened; }

    Party party() const { return m_party; }
    Privacy privacy() const { return m_privacy; }

    static QList<SIPRemotePartyId> parse(const QStringList &headers);

    static QString privacyToString(Privacy privacy);

private:
    static SIPRemotePartyId parseEntry(const QString &value);
    static QStringList splitHeaderEntries(const QString &value);
    static Privacy parsePrivacy(const QString &value);
    static Party parseParty(const QString &value);

    QString m_callbackNumber;
    QString m_rawDisplayName;
    QString m_rawNumber;
    QString m_uri;

    Party m_party = Party::Unknown;
    Privacy m_privacy = Privacy::Off;

    bool m_screened = false;
    bool m_valid = false;
};
