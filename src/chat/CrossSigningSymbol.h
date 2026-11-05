#pragma once

#include <QObject>
#include <qqmlintegration.h>

class CrossSigningSymbol
{
    Q_GADGET
    QML_NAMED_ELEMENT(crossSigningSymbol)
    QML_UNCREATABLE("")

    Q_PROPERTY(QString symbol READ symbol CONSTANT FINAL)
    Q_PROPERTY(QString description READ description CONSTANT FINAL)

public:
    CrossSigningSymbol() = default;
    CrossSigningSymbol(const QString &symbol, const QString &description = "")
        : m_symbol{ symbol }, m_description{ description }
    {
    }

    CrossSigningSymbol(const CrossSigningSymbol &) = default;
    CrossSigningSymbol &operator=(const CrossSigningSymbol &) = default;

    CrossSigningSymbol(CrossSigningSymbol &&) noexcept = default;
    CrossSigningSymbol &operator=(CrossSigningSymbol &&) noexcept = default;

    Q_INVOKABLE QString symbol() const { return m_symbol; }
    Q_INVOKABLE QString description() const { return m_description; }

private:
    QString m_symbol;
    QString m_description;
};
