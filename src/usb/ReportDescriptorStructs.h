#pragma once
#include <QDebug>
#include "ReportDescriptorEnums.h"

using UsageId = ReportDescriptorEnums::UsageId;
using UsageType = ReportDescriptorEnums::UsageType;

struct Usage
{
    UsageId id = UsageId::Unknown;
    quint32 rawId = 0;
    quint32 rawPageId = 0;
    UsageType type = UsageType::Unknown;

    quint32 min = 0;
    quint32 max = 0;
    quint32 size = 0;

    bool isConstant = false;
    bool isVariable = false;
    bool isRelative = false;
    bool isWrap = false;
    bool isBitField = false;
};

struct Report
{
    quint32 id = 0;
    QList<Usage *> usages;

    virtual ~Report();

    Usage *usage(const UsageId id) const;
    qsizetype bitFieldLength() const;
    qsizetype bitPositionForUsage(const UsageId id) const;
};

struct UsageResult
{
    Report *report = nullptr;
    Usage *usage = nullptr;
    qsizetype bitPositionInReport = -1;

    bool isValid() const { return report && usage && bitPositionInReport >= 0; }
};

struct ApplicationCollection
{
    quint32 rawPageId = 0;
    quint32 rawUsageId = 0;
    QList<Report *> reports;

    virtual ~ApplicationCollection();

    UsageResult findUsage(UsageId usageId) const;
};

QDebug operator<<(QDebug debug, const ApplicationCollection &applicationCollection);
QDebug operator<<(QDebug debug, const Report &report);
QDebug operator<<(QDebug debug, const Usage &usage);

struct ItemStateTable
{
    quint32 usagePage = 0;
    quint32 usageId = 0;
    quint32 usageMin = 0;
    quint32 usageMax = 0;
    quint32 logicalMin = 0;
    quint32 logicalMax = 0;
    quint32 reportSize = 0;
    quint32 reportCount = 0;
};
