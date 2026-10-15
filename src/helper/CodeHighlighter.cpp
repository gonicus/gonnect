#include "CodeHighlighter.h"
#include "Theme.h"
#include <KSyntaxHighlighting/Theme>
CodeHighlighter::CodeHighlighter(QObject *parent) : KSyntaxHighlighting::SyntaxHighlighter{ parent }
{
    connect(&Theme::instance(), &Theme::isDarkModeChanged, this, &CodeHighlighter::updateTheme);
    updateTheme();

    connect(this, &CodeHighlighter::languageChanged, this,
            [this]() { setDefinition(repository().definitionForName(m_language)); });

    connect(this, &CodeHighlighter::quickDocumentChanged, this,
            [this]() { setDocument(m_quickDocument->textDocument()); });
}

KSyntaxHighlighting::Repository &CodeHighlighter::repository()
{
    static KSyntaxHighlighting::Repository repo;
    return repo;
}

void CodeHighlighter::updateTheme()
{
    if (Theme::instance().isDarkMode()) {
        setTheme(repository().defaultTheme(KSyntaxHighlighting::Repository::DarkTheme));
    } else {
        setTheme(repository().defaultTheme(KSyntaxHighlighting::Repository::LightTheme));
    }
}
