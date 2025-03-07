#include <QDebug>
#include <QLoggingCategory>
#include <QStack>
#include <cmath>
#include "ReportDescriptorParser.h"

#include "hid/rdf/descriptor_view.hpp"
#include "hid/rdf/parser.hpp"

Q_LOGGING_CATEGORY(lcParser, "gonnect.usb.parser")

ReportDescriptorParser::ReportDescriptorParser(QObject *parent) : QObject(parent) { }

using desc_view = hid::rdf::ce_descriptor_view;

class Parser : public hid::rdf::parser<desc_view::iterator>
{
public:
    constexpr Parser(const desc_view &desc_view) : hid::rdf::parser<desc_view::iterator>()
    {
        hid::rdf::parser<desc_view::iterator>::parse_items(desc_view);
    }
};

std::shared_ptr<ApplicationCollection> ReportDescriptorParser::parse(QByteArray data)
{
    const hid::rdf::descriptor_view descView(
            reinterpret_cast<const unsigned char *>(data.constData()), data.size());
    const Parser p(descView);

    typedef hid::rdf::global::tag GlobalTag;
    typedef hid::rdf::main::tag MainTag;
    typedef hid::rdf::local::tag LocalTag;

    static const QList<hid::rdf::main::tag> usageTypeTags = { MainTag::INPUT, MainTag::OUTPUT,
                                                              MainTag::FEATURE };

    QStack<ItemStateTable> itemStateTableStack;
    ItemStateTable itemStateTable;

    quint8 collectionLevel = 0;
    quint32 itemCount = 0;

    QList<Usage *> pendingUsages;
    Report *report = nullptr;
    std::shared_ptr<ApplicationCollection> appColl = nullptr;

    for (auto it = descView.begin(); it != descView.end(); ++it) {
        const bool isGlobal = it->type() == hid::rdf::item_type::GLOBAL;
        const bool isMain = it->type() == hid::rdf::item_type::MAIN;
        const bool isLocal = it->type() == hid::rdf::item_type::LOCAL;
        const quint32 val = it->value_unsigned();

        if (!appColl && collectionLevel && !isGlobal && (!isMain || it->main_tag() != MainTag::END_COLLECTION)) {
            // Inside a different collection - do not care
            continue;
        }

        if (isGlobal && it->global_tag() == GlobalTag::PUSH) {
            itemStateTableStack.push(itemStateTable);
        } else if (isGlobal && it->global_tag() == GlobalTag::POP) {
            if (itemStateTableStack.isEmpty()) {
                qCFatal(lcParser)
                        << "Error: trying to opop from item state table stack although it is empty";
            }
            itemStateTable = itemStateTableStack.pop();
        } else if (isGlobal && it->global_tag() == GlobalTag::REPORT_ID) {
            if (appColl) {
                report = new Report;
                report->id = val;
                appColl->reports.append(report);
            }
        } else if (isGlobal && it->global_tag() == GlobalTag::USAGE_PAGE) {
            itemStateTable.usagePage = val;
        } else if (isMain && it->main_tag() == MainTag::COLLECTION) {

            if (!collectionLevel && val == 0x01 // Main level application collection
                && itemStateTable.usagePage == 0x0B // Usage Page Telephony
                && itemStateTable.usageId == 0x05 // Usage Headset
            ) {
                appColl = std::make_shared<ApplicationCollection>();
                appColl->rawPageId = itemStateTable.usagePage;
                appColl->rawUsageId = itemStateTable.usageId;
            }

            ++collectionLevel;
        } else if (isMain && it->main_tag() == MainTag::END_COLLECTION) {
            --collectionLevel;
            if (!collectionLevel && appColl) {
                return appColl; // Job done
            }
        } else if (isGlobal && it->global_tag() == GlobalTag::LOGICAL_MINIMUM) {
            itemStateTable.logicalMin = val;
        } else if (isGlobal && it->global_tag() == GlobalTag::LOGICAL_MAXIMUM) {
            itemStateTable.logicalMax = val;
        } else if (isLocal && it->local_tag() == LocalTag::USAGE) {
            itemStateTable.usageId = val;

            if (appColl) {
                auto usage = new Usage;
                usage->rawId = val;
                usage->id =
                        ReportDescriptorEnums::intToUsageId(itemStateTable.usagePage << 8 | val);
                usage->rawPageId = itemStateTable.usagePage;
                usage->min = itemStateTable.logicalMin;
                usage->max = itemStateTable.logicalMax;
                pendingUsages.append(usage);
            }
        } else if (isGlobal && it->global_tag() == GlobalTag::REPORT_SIZE) {
            itemStateTable.reportSize = val;
        } else if (isGlobal && it->global_tag() == GlobalTag::REPORT_COUNT) {
            itemStateTable.reportCount = val;
        } else if (isLocal && it->local_tag() == LocalTag::USAGE_MINIMUM) {
            itemStateTable.usageMin = val;
        } else if (isLocal && it->local_tag() == LocalTag::USAGE_MAXIMUM) {
            itemStateTable.usageMax = val;
        } else if (collectionLevel && isMain && usageTypeTags.contains(it->main_tag())) {
            if (pendingUsages.size()) {
                for (auto usage : std::as_const(pendingUsages)) {

                    switch (it->main_tag()) {
                    case hid::rdf::main::tag::INPUT:
                        usage->type = UsageType::Input;
                        break;
                    case hid::rdf::main::tag::OUTPUT:
                        usage->type = UsageType::Output;
                        break;
                    case hid::rdf::main::tag::FEATURE:
                        usage->type = UsageType::Feature;
                        break;
                    default:
                        usage->type = UsageType::Unknown;
                        break;
                    }

                    usage->size = itemStateTable.reportSize;
                    usage->isConstant = val & (1 << 0);
                    usage->isVariable = val & (1 << 1);
                    usage->isRelative = val & (1 << 2);
                    usage->isWrap = val & (1 << 3);
                    usage->isBitField = (val & (1 << 8)) == 0;
                    report->usages.append(usage);
                }
            } else {
                auto usage = new Usage;

                switch (it->main_tag()) {
                case hid::rdf::main::tag::INPUT:
                    usage->type = UsageType::Input;
                    break;
                case hid::rdf::main::tag::OUTPUT:
                    usage->type = UsageType::Output;
                    break;
                case hid::rdf::main::tag::FEATURE:
                    usage->type = UsageType::Feature;
                    break;
                default:
                    usage->type = UsageType::Unknown;
                    break;
                }

                usage->size = itemStateTable.reportSize * itemStateTable.reportCount;
                usage->isConstant = val & (1 << 0);
                usage->isVariable = val & (1 << 1);
                usage->isRelative = val & (1 << 2);
                usage->isWrap = val & (1 << 3);
                usage->isBitField = (val & (1 << 8)) == 0;
                report->usages.append(usage);
            }

            pendingUsages.clear();
        } else {
            qCDebug(lcParser).noquote().nospace()
                    << "Unhandled item (index " << itemCount << ": " << itemToString(it) << ", 0x"
                    << QString::number(val, 16).toUpper() << ")";
        }

        ++itemCount;
    }

    return appColl;
}

QString ReportDescriptorParser::itemToString(hid::rdf::reinterpret_iterator &it) const
{
    if (it->type() == hid::rdf::item_type::GLOBAL) {
        switch (it->global_tag()) {

        case hid::rdf::global::tag::USAGE_PAGE:
            return "[G] USAGE_PAGE";
        case hid::rdf::global::tag::LOGICAL_MINIMUM:
            return "[G] LOGICAL_MINIMUM";
        case hid::rdf::global::tag::LOGICAL_MAXIMUM:
            return "[G] LOGICAL_MAXIMUM";
        case hid::rdf::global::tag::PHYSICAL_MINIMUM:
            return "[G] PHYSICAL_MINIMUM";
        case hid::rdf::global::tag::PHYSICAL_MAXIMUM:
            return "[G] PHYSICAL_MAXIMUM";
        case hid::rdf::global::tag::UNIT_EXPONENT:
            return "[G] UNIT_EXPONENT";
        case hid::rdf::global::tag::UNIT:
            return "[G] UNIT";
        case hid::rdf::global::tag::REPORT_SIZE:
            return "[G] REPORT_SIZE";
        case hid::rdf::global::tag::REPORT_ID:
            return "[G] REPORT_ID";
        case hid::rdf::global::tag::REPORT_COUNT:
            return "[G] REPORT_COUNT";
        case hid::rdf::global::tag::PUSH:
            return "[G] PUSH";
        case hid::rdf::global::tag::POP:
            return "[G] POP";
        }
    } else if (it->type() == hid::rdf::item_type::MAIN) {
        switch (it->main_tag()) {

        case hid::rdf::main::tag::INPUT:
            return "[M] INPUT";
        case hid::rdf::main::tag::OUTPUT:
            return "[M] OUTPUT";
        case hid::rdf::main::tag::FEATURE:
            return "[M] FEATURE";
        case hid::rdf::main::tag::COLLECTION:
            return "[M] COLLECTION";
        case hid::rdf::main::tag::END_COLLECTION:
            return "[M] END_COLLECTION";
        }
    } else if (it->type() == hid::rdf::item_type::LOCAL) {
        switch (it->local_tag()) {

        case hid::rdf::local::tag::USAGE:
            return "[L] USAGE";
        case hid::rdf::local::tag::USAGE_MINIMUM:
            return "[L] USAGE_MINIMUM";
        case hid::rdf::local::tag::USAGE_MAXIMUM:
            return "[L] USAGE_MAXIMUM";
        case hid::rdf::local::tag::DESIGNATOR_INDEX:
            return "[L] DESIGNATOR_INDEX";
        case hid::rdf::local::tag::DESIGNATOR_MINIMUM:
            return "[L] DESIGNATOR_MINIMUM";
        case hid::rdf::local::tag::DESIGNATOR_MAXIMUM:
            return "[L] DESIGNATOR_MAXIMUM";
        case hid::rdf::local::tag::STRING_INDEX:
            return "[L] STRING_INDEX";
        case hid::rdf::local::tag::STRING_MINIMUM:
            return "[L] STRING_MINIMUM";
        case hid::rdf::local::tag::STRING_MAXIMUM:
            return "[L] STRING_MAXIMUM";
        case hid::rdf::local::tag::DELIMITER:
            return "[L] DELIMITER";
        }
    } else if (it->type() == hid::rdf::item_type::RESERVED) {
        return "[R] Reserved";
    }
    return "[?] Unknown";
}
