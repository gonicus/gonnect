#pragma once

#include <QObject>
#include <qqmlintegration.h>
#include "CrossSigningSymbol.h"

class CrossSigningSecret
{
    Q_GADGET
    QML_NAMED_ELEMENT(crossSigningSecret)
    QML_UNCREATABLE("")

public:
    enum class CrossSigningMethod { SasString, SasSymbol };
    Q_ENUM(CrossSigningMethod)

    CrossSigningSecret() { }
    CrossSigningSecret(const CrossSigningSecret &other) = default;
    CrossSigningSecret &operator=(const CrossSigningSecret &other) = default;
    CrossSigningSecret(CrossSigningSecret &&) noexcept = default;
    CrossSigningSecret &operator=(CrossSigningSecret &&) noexcept = default;

    Q_INVOKABLE CrossSigningSecret::CrossSigningMethod method() const { return m_method; }
    Q_INVOKABLE QString stringSecret() const { return m_stringSecret; }
    Q_INVOKABLE QList<CrossSigningSymbol> symbolSequence() const { return m_symbolSequence; }

    void setMethod(CrossSigningMethod method) { m_method = method; }
    void setStringSecret(const QString &secret) { m_stringSecret = secret; }
    void setSymbolSeqence(const QList<CrossSigningSymbol> &sequence)
    {
        m_symbolSequence = sequence;
    }

private:
    CrossSigningSecret::CrossSigningMethod m_method = CrossSigningMethod::SasString;
    QString m_stringSecret;
    QList<CrossSigningSymbol> m_symbolSequence;
};

namespace CrossSigningSecretForeign {
Q_NAMESPACE
QML_NAMED_ELEMENT(CrossSigningSecret)
QML_FOREIGN_NAMESPACE(CrossSigningSecret)
} // namespace CrossSigningSecretForeign
