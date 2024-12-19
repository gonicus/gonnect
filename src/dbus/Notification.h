#pragma once

#include <QObject>
#include <QFileInfo>

class Notification : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("RegisterEnumClassesUnscoped", "false")
    Q_DISABLE_COPY(Notification)

public:
    enum Priority { low, normal, high, urgent };
    Q_ENUM(Priority)

    enum DisplayHint {
        persistent = 1,
        transient = 2,
        tray = 4,
        hideOnLockscreen = 8,
        hideContentOnLockScreen = 16,
        showAsNew = 32
    };
    Q_ENUM(DisplayHint)

    explicit Notification(const QString &title, const QString &body,
                          Priority priority = Priority::normal, QObject *parent = nullptr);
    ~Notification();

    // TODO: set display-hint
    //  display-hint: persistent, transient / tray, hide-on-lockscreen, hide-content-on-lockscreen,
    //  show-as-new

    QString id() const { return m_id; }
    void setVersion(unsigned version) { m_version = version; }

    // Configure what action is emitted if the notification is clicked
    void setDefaultAction(const QString &action) { m_defaultAction = action; }
    void setDefaultActionParameters(const QVariantList &parameters)
    {
        m_defaultParameters = parameters;
    }

    void setIcon(const QString &iconUri);
    void setSound(const QString &sound) { m_sound = sound; }
    void setCategory(const QString &category) { m_category = category; }
    void setDisplayHint(unsigned hint) { m_displayHint = hint; }
    void setRoundedIcon(bool flag);
    void setEmblem(const QString &emblemUri);
    void addButton(const QString &label, const QString &action, const QString &purpose,
                   const QVariantMap &parameters);

    QVariantMap toPortalDefinition() const;
    QVariantMap toPortalDefinition2() const;

signals:
    void actionInvoked(QString action, QVariantList parameters);

private:
    QString priority() const;

    QVariantList m_defaultParameters;
    QList<QVariantMap> m_buttons;

    QString m_id;
    QString m_title;
    QString m_body;
    QString m_defaultAction;
    QString m_iconUri;
    QString m_emblemUri;
    QString m_sound = "silent";
    QString m_category;

    unsigned m_displayHint = 0;
    unsigned m_version = 1;

    bool m_roundedIcon = false;

    Priority m_priority = Priority::normal;
};
