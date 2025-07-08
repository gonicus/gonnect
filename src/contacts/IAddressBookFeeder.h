#pragma once
#include <QString>
#include <QUrl>

class IAddressBookFeeder
{
public:
    virtual void process() = 0;
    virtual QUrl networkCheckURL() const { return QUrl(); }

protected:
    QString m_displayName;
    unsigned m_priority = 0;
};
