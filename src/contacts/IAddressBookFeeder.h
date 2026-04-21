#pragma once
#include <QString>
#include <QUrl>

class IAddressBookFeeder
{
public:
    virtual void process() = 0;
    virtual QUrl networkCheckURL() const { return QUrl(); }

    bool isProcessing() const { return m_isProcessing; }
    bool isInitialized() const { return m_isInitialized; }

protected:
    QString m_displayName;
    unsigned m_priority = 0;
    bool m_isProcessing = false;
    bool m_isInitialized = false;
};
