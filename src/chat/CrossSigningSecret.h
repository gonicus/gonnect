#pragma once

#include <QObject>
#include <qqmlintegration.h>

class CrossSigningSymbol : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

public:
    CrossSigningSymbol(const QString &symbol, const QString &description = "",
                       QObject *parent = nullptr)
        : QObject{ parent }, m_symbol{ symbol }, m_description{ description }
    {
    }

    Q_INVOKABLE QString symbol() const { return m_symbol; }
    Q_INVOKABLE QString description() const { return m_description; }

private:
    QString m_symbol;
    QString m_description;
};

class CrossSigningSecret : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

public:
    enum class CrossSigningMethod { SasString, SasSymbol };
    Q_ENUM(CrossSigningMethod)

    CrossSigningSecret(QObject *parent = nullptr) : QObject{ parent } { }
    CrossSigningSecret(const CrossSigningSecret &other) : QObject(other.parent())
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

    Q_INVOKABLE CrossSigningSecret::CrossSigningMethod method() const { return m_method; }
    Q_INVOKABLE QString stringSecret() const { return m_stringSecret; }
    Q_INVOKABLE ::QList<::CrossSigningSymbol *> symbolSequence() const { return m_symbolSequence; }

    void setMethod(CrossSigningMethod method) { m_method = method; }
    void setStringSecret(const QString &secret) { m_stringSecret = secret; }
    void setSymbolSeqence(const QList<CrossSigningSymbol *> &sequence)
    {
        m_symbolSequence = sequence;
        for (auto symbol : std::as_const(sequence)) {
            symbol->setParent(this);
        }
    }

private:
    CrossSigningSecret::CrossSigningMethod m_method = CrossSigningMethod::SasString;
    QString m_stringSecret;
    QList<CrossSigningSymbol *> m_symbolSequence;
};
