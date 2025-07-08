#include "KeychainSettings.h"

QString KeychainSettings::secret(const QString &group)
{
    KeychainSettings keychainSettings;
    keychainSettings.beginGroup(group);
    return keychainSettings.value("secret", "").toString();
}
