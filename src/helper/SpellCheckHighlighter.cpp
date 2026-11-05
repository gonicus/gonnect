#include "SpellCheckHighlighter.h"
#include <hunspell/hunspell.hxx>
#include <QRegularExpression>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcSpellCheck, "gonnect.app.SpellCheck")

SpellCheckHighlighter::SpellCheckHighlighter(QObject *parent) : QSyntaxHighlighter{ parent }
{
    connect(this, &SpellCheckHighlighter::quickDocumentChanged, this,
            [this]() { setDocument(m_quickDocument ? m_quickDocument->textDocument() : nullptr); });

    m_hunspell = new Hunspell("/usr/share/hunspell/de_DE.aff", "/usr/share/hunspell/de_DE.dic");
    m_encoder = QStringEncoder(m_hunspell->get_dict_encoding());

    if (!m_encoder.isValid()) {
        qCCritical(lcSpellCheck) << "Unable to encode string for hunspell dict encoding"
                                 << m_hunspell->get_dict_encoding();
    }

    m_badSpellFormat.setUnderlineStyle(QTextCharFormat::SingleUnderline);
}

void SpellCheckHighlighter::highlightBlock(const QString &text)
{
    if (!m_encoder.isValid()) {
        return;
    }

    static const QRegularExpression wordMatcher(R"(\p{L}+)");

    const auto matches = wordMatcher.globalMatchView(text);

    for (const auto &match : matches) {
        const auto str = match.captured();
        const std::string encStr =
                static_cast<QByteArray>(m_encoder(match.captured())).toStdString();

        if (!str.isEmpty() && !m_hunspell->spell(encStr)) {
            setFormat(match.capturedStart(), match.capturedLength(), m_badSpellFormat);
        }
    }
}
