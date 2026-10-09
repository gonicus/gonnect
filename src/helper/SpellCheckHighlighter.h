#pragma once

#include <QSyntaxHighlighter>
#include <QQuickTextDocument>
#include <QQmlEngine>
#include <QStringEncoder>

class Hunspell;

class SpellCheckHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QQuickTextDocument *quickDocument MEMBER m_quickDocument NOTIFY quickDocumentChanged
                       FINAL)

public:
    explicit SpellCheckHighlighter(QObject *parent = nullptr);

protected:
    virtual void highlightBlock(const QString &text) override;

private:
    QQuickTextDocument *m_quickDocument = nullptr;
    QStringEncoder m_encoder;
    QTextCharFormat m_badSpellFormat;
    Hunspell *m_hunspell = nullptr;

Q_SIGNALS:
    void quickDocumentChanged();
};
