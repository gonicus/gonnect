#pragma once

class AddressBook;

class IAddressBookFeeder
{

public:
    virtual void feedAddressBook(AddressBook &addressBook) = 0;
};
