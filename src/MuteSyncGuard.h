#pragma once
#include <QString>
#include <QUuid>

class MuteSyncGuard
{
public:
    QString originate()
    {
        m_pendingTag = QUuid::createUuid().toString();
        return m_pendingTag;
    }

    bool isOwnEcho(const QString &tag)
    {
        if (!m_pendingTag.isEmpty() && m_pendingTag == tag) {
            m_pendingTag.clear();
            return true;
        }
        return false;
    }

private:
    QString m_pendingTag;
};
