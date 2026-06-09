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
