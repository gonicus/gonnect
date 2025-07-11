#pragma once
#include <QObject>
#include <hid/rdf/descriptor_view.hpp>

#include "ReportDescriptorStructs.h"

class ReportDescriptorParser : public QObject
{
    Q_OBJECT

public:
    explicit ReportDescriptorParser(QObject *parent = nullptr);

    std::shared_ptr<ApplicationCollection> parse(const QByteArray &data);
    QHash<ReportDescriptorEnums::UsageId, quint16> parseTeamsReportIDs(const QByteArray &data);

private:
    QString usagePageToString(quint16 page);
    QString usageToString(quint16 page, quint16 usage);
    QString dataItemToString(quint16 data);
    QString itemToString(hid::rdf::reinterpret_iterator &it) const;
};
