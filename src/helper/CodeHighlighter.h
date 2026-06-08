#pragma once

#include <QSyntaxHighlighter>
#include <QQuickTextDocument>
#include <QQmlEngine>

#include <KSyntaxHighlighting/SyntaxHighlighter>
#include <KSyntaxHighlighting/Repository>

class CodeHighlighter : public KSyntaxHighlighting::SyntaxHighlighter
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QQuickTextDocument *quickDocument MEMBER m_quickDocument NOTIFY quickDocumentChanged
                       FINAL)
    Q_PROPERTY(QString language MEMBER m_language NOTIFY languageChanged FINAL)

public:
    explicit CodeHighlighter(QObject *parent = nullptr);

private:
    static KSyntaxHighlighting::Repository &repository();

    QQuickTextDocument *m_quickDocument = nullptr;
    QString m_language;

private Q_SLOTS:
    void updateTheme();

Q_SIGNALS:
    void quickDocumentChanged();
    void languageChanged();
};
