#include "MSGraphAddressBookFactory.h"
#include "MSGraphAddressBookFeeder.h"
#include "ReadOnlyConfdSettings.h"

QStringList MSGraphAddressBookFactory::configurations() const
{
    QStringList res;

    ReadOnlyConfdSettings settings;
    const auto msOAuthGroup = QStringLiteral("msoauth");
    const auto group = QStringLiteral("msgraphcontacts");
    if (settings.childGroups().contains(msOAuthGroup) && settings.childGroups().contains(group)) {
        const auto &clientIdentifier =
                settings.value(msOAuthGroup + QStringLiteral("/clientIdentifier")).toString();
        const bool enabled = settings.value(group + QStringLiteral("enabled"), true).toBool();

        if (enabled && !clientIdentifier.isEmpty()) {
            res.push_back(group);
        }
    }

    return res;
}

IAddressBookFeeder *MSGraphAddressBookFactory::createFeeder(const QString &id,
                                                            AddressBookManager *parent) const
{
    return new MSGraphAddressBookFeeder(id, parent);
}
