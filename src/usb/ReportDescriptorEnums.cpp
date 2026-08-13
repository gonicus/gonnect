#include "ReportDescriptorEnums.h"
#include <QMetaEnum>
#include <utility>

QString ReportDescriptorEnums::toString(const UsageId id)
{
    return QMetaEnum::fromType<UsageId>().valueToKey(std::to_underlying(id));
}

QString ReportDescriptorEnums::toString(const TeamsDisplayFieldSupport id)
{
    return QMetaEnum::fromType<TeamsDisplayFieldSupport>().valueToKey(std::to_underlying(id));
}

QString ReportDescriptorEnums::toString(const TeamsScreenSelect id)
{
    return QMetaEnum::fromType<TeamsScreenSelect>().valueToKey(std::to_underlying(id));
}

QString ReportDescriptorEnums::toString(const TeamsPresenceIcon id)
{
    return QMetaEnum::fromType<TeamsPresenceIcon>().valueToKey(std::to_underlying(id));
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
    return QMetaEnum::fromType<UsageType>().valueToKey(std::to_underlying(id));
}
