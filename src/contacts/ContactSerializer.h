#pragma once

#include <QObject>

class AddressBook;

class ContactSerializer : public QObject
{
    Q_OBJECT

public:
    ContactSerializer(const QString &filePath, QObject *parent = nullptr);

    void saveAddressBook(const AddressBook &addressBook);
    void loadAddressBook(AddressBook &addressBook);

private:
    QString m_filePath;
};
