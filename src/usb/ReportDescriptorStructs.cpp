#include "ReportDescriptorStructs.h"

Report::~Report()
{
    qDeleteAll(usages);
    usages.clear();
}

Usage *Report::usage(const UsageId id) const
{
    for (auto usage : std::as_const(usages)) {
        if (usage->id == id) {
            return usage;
        }
    }
    return nullptr;
}

qsizetype Report::bitFieldLength() const
{
    qsizetype l = 0;

    for (const auto usage : std::as_const(usages)) {
        l += usage->size;
    }

    return l;
}

qsizetype Report::bitPositionForUsage(const UsageId id) const
{
    qsizetype acc = 0;

    for (const auto usage : std::as_const(usages)) {
        if (usage->id == id) {
            return acc;
        }

        acc += usage->size;
    }

    return -1;
}

QDebug operator<<(QDebug debug, const ApplicationCollection &applicationCollection)
{
    QDebugStateSaver saver(debug);
    debug.nospace().noquote() << "ApplicationCollection (Usage Page 0x"
                              << QString::number(applicationCollection.rawPageId, 16).toUpper()
                              << ", Usage: 0x"
                              << QString::number(applicationCollection.rawUsageId, 16).toUpper()
                              << ")\n";

    for (const auto report : std::as_const(applicationCollection.reports)) {
        debug << "  " << *report << "\n";
    }

    return debug;
}

QDebug operator<<(QDebug debug, const Report &report)
{
    QDebugStateSaver saver(debug);
    debug.nospace().noquote() << "Report (0x" << QString::number(report.id, 16).toUpper() << ", "
                              << report.usages.size() << " usages)\n";

    for (const auto usage : std::as_const(report.usages)) {
        debug << "    " << *usage << "\n";
    }

    return debug;
}

QDebug operator<<(QDebug debug, const Usage &usage)
{
    QDebugStateSaver saver(debug);

    QStringList config;
    config.append(usage.isConstant ? "Constant" : "Data");
    config.append(usage.isVariable ? "Variable" : "Array");
    config.append(usage.isRelative ? "Relative" : "Absolute");
    if (usage.isWrap) {
        config.append("Wrap");
    }
    config.append(usage.isBitField ? "Bit Field" : "Buffered Bytes");

    const QString usageStr = QString("%1 (0x%2)")
                                     .arg(ReportDescriptorEnums::toString(usage.id),
                                          QString::number(static_cast<quint32>(usage.rawId), 16)
                                                  .toUpper()
                                                  .leftJustified(4, '0'));

    debug.nospace().noquote() << "Usage (" << usageStr << ", "
                              << ReportDescriptorEnums::toString(usage.type)
                              << ", size: " << usage.size << ", min: " << usage.min
                              << ", max: " << usage.max << ", " << config.join(", ") << ")";

    return debug;
}

ApplicationCollection::~ApplicationCollection()
{
    qDeleteAll(reports);
    reports.clear();
}

UsageResult ApplicationCollection::findUsage(UsageId usageId) const
{
    for (Report *report : std::as_const(reports)) {
        QHash<UsageType, qsizetype> acc;

        for (Usage *usage : std::as_const(report->usages)) {
            if (usage->id == usageId) {
                return UsageResult({ report, usage, acc.value(usage->type, 0) });
            }

            acc.insert(usage->type, acc.value(usage->type, 0) + usage->size);
        }
    }

    return UsageResult();
}
