#pragma once
#include <QString>
#include <QStringList>
#include <QObject>

// Parser for the Cisco Call-Info header (Cisco OL-25254-01)
struct SIPCallInfo
{
    Q_GADGET

public:
    // Security status of the whole call as judged by CUCM
    enum Security {
        Unknown,
        NotAuthenticated,
        Authenticated,
        Encrypted
    };
    Q_ENUM(Security)

    // Tone requested by CUCM
    enum UiState {
        None,
        Ringout,
        Busy
    };
    Q_ENUM(UiState)

    // Urgency presented by CUCM
    enum Priority {
        Normal,
        Urgent,
        Emergency
    };
    Q_ENUM(Priority)

    bool isValid() const { return m_valid; }
    Security security() const { return m_security; }
    UiState uiState() const { return m_uiState; }
    Priority priority() const { return m_priority; }
    QString gci() const { return m_gci; };

    bool priorityOverride() const { return isPriorityOverride(m_priority); }
    static bool isPriorityOverride(Priority priority) { return priority != Priority::Normal; }

    static SIPCallInfo parse(const QStringList &callInfoHeaders);

    static QString securityToString(Security security);
    static QString uiStateToString(UiState uiState);
    static QString priorityToString(Priority priority);

private:
    static QStringList splitHeaderEntries(const QString &value);
    static Security parseSecurity(const QString &value);
    static UiState parseUiState(const QString &value);
    static Priority parsePriority(const QString &value);

    bool m_valid = false;
    Security m_security = Security::Unknown;
    UiState m_uiState = UiState::None;
    QString m_gci;
    Priority m_priority = Priority::Normal;
};
