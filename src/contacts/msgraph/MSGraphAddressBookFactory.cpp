#include "MSGraphAddressBookFactory.h"
#include "MSGraphAddressBookFeeder.h"
#include "ReadOnlyConfdSettings.h"

QStringList MSGraphAddressBookFactory::configurations() const
{
    QStringList res;

    static QRegularExpression groupRegex = QRegularExpression("^msgraph[0-9]+$");

    ReadOnlyConfdSettings settings;
    const QStringList groups = settings.childGroups();

    for (const auto &group : groups) {        
        if (groupRegex.match(group).hasMatch()) {
            settings.beginGroup(group);
            const bool enabled = settings.value("enabled", true).toBool();
            settings.endGroup();

            if (enabled) {
                res.push_back(group);
            }
        }
    }

    return res;
}

IAddressBookFeeder *MSGraphAddressBookFactory::createFeeder(const QString &id,
                                                            AddressBookManager *parent) const
{
    return new MSGraphAddressBookFeeder(id, parent);
}
