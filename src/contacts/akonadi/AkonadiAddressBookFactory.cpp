#include "AkonadiAddressBookFactory.h"
#include "AkonadiAddressBookFeeder.h"
#include "ReadOnlyConfdSettings.h"

QStringList AkonadiAddressBookFactory::configurations() const
{
    const QString groupName = "akonadi-contacts";

    ReadOnlyConfdSettings settings;
    const QStringList groups = settings.childGroups();

    if (groups.contains(groupName)) {
        settings.beginGroup(groupName);
        const bool enabled = settings.value("enabled", true).toBool();
        settings.endGroup();

        if (!enabled) {
            return {};
        }
    }

    return { groupName };
}

IAddressBookFeeder *AkonadiAddressBookFactory::createFeeder(const QString &group,
                                                            AddressBookManager *parent) const
{
    return new AkonadiAddressBookFeeder(group, parent);
}
