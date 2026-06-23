#pragma once

#include <QObject>
#include <qqmlintegration.h>
#include "CrossSigningSymbol.h"

class CrossSigningSecret
{
    Q_GADGET
    QML_ELEMENT
    QML_NAMED_ELEMENT(crossSigningSecret)
    QML_UNCREATABLE("")

public:
    enum class CrossSigningMethod { SasString, SasSymbol };
    Q_ENUM(CrossSigningMethod)

    CrossSigningSecret() { }
    CrossSigningSecret(const CrossSigningSecret &other)
    {
        m_method = other.m_method;
        m_stringSecret = other.m_stringSecret;
        m_symbolSequence = other.m_symbolSequence;
    }

    CrossSigningSecret &operator=(const CrossSigningSecret &other)
    {
        m_method = other.m_method;
        m_stringSecret = other.m_stringSecret;
        m_symbolSequence = other.m_symbolSequence;
        return *this;
    }

    ~CrossSigningSecret() { qDeleteAll(m_symbolSequence); }

    Q_INVOKABLE CrossSigningSecret::CrossSigningMethod method() const { return m_method; }
    Q_INVOKABLE QString stringSecret() const { return m_stringSecret; }

    // clazy:skip
    Q_INVOKABLE QList<CrossSigningSymbol *> symbolSequence() const { return m_symbolSequence; }

    void setMethod(CrossSigningMethod method) { m_method = method; }
    void setStringSecret(const QString &secret) { m_stringSecret = secret; }
    void setSymbolSeqence(const QList<CrossSigningSymbol *> &sequence)
    {
        m_symbolSequence = sequence;
    }

private:
    CrossSigningSecret::CrossSigningMethod m_method = CrossSigningMethod::SasString;
    QString m_stringSecret;
    QList<CrossSigningSymbol *> m_symbolSequence;
};
