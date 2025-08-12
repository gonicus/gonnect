#include "JitsiMediaDevice.h"

JitsiMediaDevice::JitsiMediaDevice(const QString &deviceId, const QString &groupId, const Type type,
                                   const QString &label, QObject *parent)
    : QObject{ parent },
      m_deviceId{ deviceId },
      m_groupId{ groupId },
      m_label{ label },
      m_type{ type }
{
}
