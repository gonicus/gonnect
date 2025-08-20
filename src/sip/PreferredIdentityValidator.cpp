#include "PreferredIdentityValidator.h"

#include <QRegularExpression>

PreferredIdentityValidator::PreferredIdentityValidator(QObject *parent) : QObject{ parent } { }

bool PreferredIdentityValidator::isValid(const PreferredIdentity &preferredIdentity) const
{
    return isDisplayNameValid(preferredIdentity.displayName())
            && isIdentityNumberValid(preferredIdentity.identity())
            && isPrefixValid(preferredIdentity.prefix());
}

bool PreferredIdentityValidator::isDisplayNameValid(const QString &displayName) const
{
    return !displayName.isEmpty();
}

bool PreferredIdentityValidator::isIdentityNumberValid(const QString &identityNumber) const
{
    static const QRegularExpression numberMatch("^\\+[0-9]+$");
    return numberMatch.match(identityNumber).hasMatch();
}

bool PreferredIdentityValidator::isPrefixValid(const QString &prefix) const
{
    static const QRegularExpression mulitNumberMatch("^(\\+[0-9]+)(,\\+[0-9]+)*$");
    return prefix.isEmpty() || mulitNumberMatch.match(prefix).hasMatch();
}
