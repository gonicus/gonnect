#pragma once

#include <QObject>

class Contact;

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

    void initialLoad(const QString &ldapUrl, const QString &ldapBase, const QString &ldapFilter);

    QString avatarPathFor(const QString &id);

    void addExternalImage(const QString &id, const QByteArray &data, const QDateTime &modified);

private:
    void clearCStringlist(char **attrs) const;
    void createFile(const QString &id, const QByteArray &data) const;
    void addIdsToDb(QHash<QString, QDateTime> &idTimeMap) const;
    void updateAvatarModifiedTime(const QString &id, const QDateTime &modified) const;
    QDateTime modifiedTimeInDb(const QString &id) const;
    QHash<QString, QDateTime> readIdsFromDb() const;
    void loadAvatars(const QList<const Contact *> &contacts, const QString &ldapUrl,
                     const QString &ldapBase, const QString &ldapFilter);

    QStringList readContactIdsFromDir() const;
    void loadAll(const QString &ldapUrl, const QString &ldapBase, const QString &ldapFilter);
    explicit AvatarManager(QObject *parent = nullptr);

    QString m_avatarImageDirPath;

signals:
    void avatarsLoaded();
};
