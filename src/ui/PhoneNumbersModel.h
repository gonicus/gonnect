#pragma once

#include <QAbstractListModel>
#include <QObject>
#include <QQmlEngine>

#include "Contact.h"

class PhoneNumbersModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(Contact *contact MEMBER m_contact NOTIFY contactChanged FINAL)
    Q_PROPERTY(qsizetype count READ rowCount NOTIFY countChanged FINAL)

public:
    enum class Roles { Number = Qt::UserRole + 1, Type, IsSipSubscribable };
    explicit PhoneNumbersModel(QObject *parent = nullptr);

    Q_INVOKABLE QString soleNumber() const;

    virtual QHash<int, QByteArray> roleNames() const override;
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    virtual QVariant data(const QModelIndex &index, int role) const override;

private Q_SLOTS:
    void onContactChanged();

private:
    Contact *m_contact = nullptr;
    QObject *m_contactContext = nullptr;

Q_SIGNALS:
    void contactChanged();
    void countChanged();
};
