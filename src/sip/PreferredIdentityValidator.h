#pragma once

#include <QObject>
#include <QQmlEngine>

#include "PreferredIdentity.h"

class PreferredIdentityValidator : public QObject
{
    Q_OBJECT

public:
    static PreferredIdentityValidator &instance()
    {
        static PreferredIdentityValidator *_instance = nullptr;
        if (!_instance) {
            _instance = new PreferredIdentityValidator;
        }
        return *_instance;
    }

    bool isValid(const PreferredIdentity &preferredIdentity) const;

    Q_INVOKABLE bool isDisplayNameValid(const QString &displayName) const;
    Q_INVOKABLE bool isIdentityNumberValid(const QString &identityNumber) const;
    Q_INVOKABLE bool isPrefixValid(const QString &prefix) const;

private:
    explicit PreferredIdentityValidator(QObject *parent = nullptr);
};

class PreferredIdentityValidatorWrapper
{
    Q_GADGET
    QML_FOREIGN(PreferredIdentityValidator)
    QML_NAMED_ELEMENT(PreferredIdentityValidator)
    QML_SINGLETON

public:
    static PreferredIdentityValidator *create(QQmlEngine *, QJSEngine *)
    {
        return &PreferredIdentityValidator::instance();
    }

private:
    PreferredIdentityValidatorWrapper() = default;
};
