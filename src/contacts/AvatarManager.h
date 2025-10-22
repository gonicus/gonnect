#pragma once

#include <QObject>
#include <QTimer>
#include "Contact.h"

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

    void initialLoad();

    QString avatarPathFor(const QString &id);

    void addExternalImage(const QString &id, const QByteArray &data, const QDateTime &modified);
    void removeExternalImage(const QString &id);

private:
    void createFile(const QString &id, const QByteArray &data) const;
    void removeFile(const QString &id) const;
    void addIdsToDb(QHash<QString, QDateTime> &idTimeMap) const;
    void removeIdsFromDb(QList<QString> &idList) const;
    void updateAvatarModifiedTime(const QString &id, const QDateTime &modified) const;
    QDateTime modifiedTimeInDb(const QString &id) const;
    QHash<QString, QDateTime> readIdsFromDb() const;

    QStringList readContactIdsFromDir() const;
    explicit AvatarManager(QObject *parent = nullptr);

    QString m_avatarImageDirPath;
    QTimer m_updateContactsTimer;
    QList<QPointer<Contact>> m_contactsWithPendingUpdates;

private Q_SLOTS:
    void updateContacts();

Q_SIGNALS:
    void avatarManagerInitialized(QList<const Contact *> dirtyContacts);
    void avatarsLoaded();
    void avatarAdded(QString contactId);
    void avatarRemoved(QString contactId);
};
