#include "ReportDescriptorEnums.h"
#include <QMetaEnum>

QString ReportDescriptorEnums::toString(const UsageId id)
{
    return QMetaEnum::fromType<UsageId>().valueToKey(static_cast<int>(id));
}

QString ReportDescriptorEnums::toString(const TeamsDisplayFieldSupport id)
{
    return QMetaEnum::fromType<TeamsDisplayFieldSupport>().valueToKey(static_cast<int>(id));
}

QString ReportDescriptorEnums::toString(const TeamsScreenSelect id)
{
    return QMetaEnum::fromType<TeamsScreenSelect>().valueToKey(static_cast<int>(id));
}

QString ReportDescriptorEnums::toString(const TeamsPresenceIcon id)
{
    return QMetaEnum::fromType<TeamsPresenceIcon>().valueToKey(static_cast<int>(id));
}

ReportDescriptorEnums::UsageId ReportDescriptorEnums::intToUsageId(const quint32 id)
{
    const auto metaEnum = QMetaEnum::fromType<UsageId>();
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        const auto value = metaEnum.value(i);
        if (static_cast<quint32>(value) == id) {
            return static_cast<UsageId>(id);
        }
    }
    return UsageId::Unknown;
}

QString ReportDescriptorEnums::toString(const UsageType id)
{
    return QMetaEnum::fromType<UsageType>().valueToKey(static_cast<int>(id));
}
