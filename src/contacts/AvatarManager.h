#pragma once

#include <QObject>
#include <QTimer>
#include "Contact.h"
#include "LDAPInitializer.h"

class AvatarManager : public QObject
{
    Q_OBJECT

public:
    static AvatarManager &instance()
    {
        static AvatarManager *_instance = nullptr;
        if (!_instance) {
            _instance = new AvatarManager;
        }
        return *_instance;
    }

    void initialLoad(const LDAPInitializer::Config &ldapConfig);

    QString avatarPathFor(const QString &id);

    void addExternalImage(const QString &id, const QByteArray &data, const QDateTime &modified);
    void removeExternalImage(const QString &id);

private:
    void clearCStringlist(char **attrs) const;
    void createFile(const QString &id, const QByteArray &data) const;
    void removeFile(const QString &id) const;
    void addIdsToDb(QHash<QString, QDateTime> &idTimeMap) const;
    void removeIdsFromDb(QList<QString> &idList) const;
    void updateAvatarModifiedTime(const QString &id, const QDateTime &modified) const;
    QDateTime modifiedTimeInDb(const QString &id) const;
    QHash<QString, QDateTime> readIdsFromDb() const;
    void loadAvatars(const QList<const Contact *> &contacts,
                     const LDAPInitializer::Config &ldapConfig);

    QStringList readContactIdsFromDir() const;
    void loadAll(const LDAPInitializer::Config &ldapConfig);
    explicit AvatarManager(QObject *parent = nullptr);

    QString m_avatarImageDirPath;
    QTimer m_updateContactsTimer;
    QList<QPointer<Contact>> m_contactsWithPendingUpdates;

private slots:
    void updateContacts();

signals:
    void avatarsLoaded();
    void avatarAdded(QString contactId);
    void avatarRemoved(QString contactId);
};
