#pragma once

#include <QAbstractListModel>
#include <QObject>
#include <QQmlEngine>

class Contact;
struct NumberStat;

class SearchListModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString searchPhrase MEMBER m_searchPhrase NOTIFY searchPhraseChanged FINAL)
    Q_PROPERTY(quint16 totalNumbersCount READ totalNumbersCount NOTIFY numbersIndexUpdated FINAL)

public:
    enum class Roles {
        Id = Qt::UserRole + 1,
        Name,
        Company,
        HasAvatar,
        AvatarPath,
        SubscriptableNumber,
        Numbers,
        NumbersCount,
        NumbersIndexOffset,
        SourceDisplayName,
        SourcePriority
    };

    explicit SearchListModel(QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    quint16 totalNumbersCount() const { return m_totalNumbersCount; }

    Q_INVOKABLE QString phoneNumberByIndex(quint16 index) const;
    Q_INVOKABLE QString contactIdByIndex(quint16 index) const;

private Q_SLOTS:
    void updateSearchResults();
    void handleFavoriteToggle(const NumberStat *numberStat);

private:
    QString m_searchPhrase;
    QList<Contact *> m_model;
    QList<quint8> m_numberIndexOffsets;
    quint16 m_totalNumbersCount = 0;

Q_SIGNALS:
    void searchPhraseChanged();
    void numbersIndexUpdated();
};
