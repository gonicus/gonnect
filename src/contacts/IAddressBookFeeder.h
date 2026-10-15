#pragma once
#include <atomic>
#include <QString>
#include <QUrl>

class IAddressBookFeeder
{
public:
    virtual void process() = 0;
    virtual QUrl networkCheckURL() const { return QUrl(); }

    bool isProcessing() const { return m_isProcessing; }

protected:
    QString m_displayName;
    unsigned m_priority = 0;
    std::atomic<bool> m_isProcessing = false;
};
