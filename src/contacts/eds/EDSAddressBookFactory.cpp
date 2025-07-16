#include "EDSAddressBookFactory.h"
#include "ReadOnlyConfdSettings.h"

QStringList EDSAddressBookFactory::configurations() const
{
    const QString groupName = "eds-contacts";

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

IAddressBookFeeder *EDSAddressBookFactory::createFeeder(const QString &id,
                                                        AddressBookManager *parent) const
{
    return new EDSAddressBookFeeder(id, parent);
}
