#include <QLoggingCategory>

#include "ChatMessageSearchPreprocessor.h"

Q_LOGGING_CATEGORY(lcChatMessageSearchPreprocessor, "gonnect.chat.message.search.preprocessor")

ChatMessageSearchPreprocessor::ChatMessageSearchPreprocessor(QObject *parent) : QObject(parent)
{
    UErrorCode err = U_ZERO_ERROR;
    UParseError pe = {};
    QString transliterationRules = tr(":: Lower;"
                                      ":: NFD;"
                                      ":: [:Nonspacing Mark:] Remove;"
                                      ":: NFC;");

    // If ICU setup fails we'll continue without transliteration
    m_transliterator = icu::Transliterator::createFromRules(
            "de-norm", icu::UnicodeString(transliterationRules.toUtf8(), "UTF-8"), UTRANS_FORWARD,
            pe, err);
    if (U_FAILURE(err)) {
        qCWarning(lcChatMessageSearchPreprocessor)
                << "Error during ICU initialization:" << u_errorName(err);
    }

    // Snowball stemmers for German and English (UTF-8 encoding).
    m_stemmerDe = sb_stemmer_new("german", "UTF_8");
    m_stemmerEn = sb_stemmer_new("english", "UTF_8");
    if (!m_stemmerDe || !m_stemmerEn) {
        qCWarning(lcChatMessageSearchPreprocessor) << "Failed to initialize stemmers";
        return;
    }

    m_isInitialized = true;
}

ChatMessageSearchPreprocessor::~ChatMessageSearchPreprocessor()
{
    if (m_transliterator) {
        delete m_transliterator;
        m_transliterator = nullptr;
    }

    if (m_stemmerDe) {
        sb_stemmer_delete(m_stemmerDe);
        m_stemmerDe = NULL;
    }

    if (m_stemmerEn) {
        sb_stemmer_delete(m_stemmerEn);
        m_stemmerEn = NULL;
    }
}

QString ChatMessageSearchPreprocessor::process(const QString &text)
{
    QMutexLocker locker(&m_preprocessorMutex);

    // Step 1 - ICU normalisation: Convert to ICU UnicodeString
    // apply the transliterator, convert back
    const QByteArray utf8In = text.toUtf8();
    icu::UnicodeString us =
            icu::UnicodeString::fromUTF8(icu::StringPiece(utf8In.constData(), utf8In.size()));

    if (m_transliterator) {
        m_transliterator->transliterate(us);
    }

    std::string utf8Out;
    us.toUTF8String(utf8Out);

    // Step 2 & 3: Per-token Snowball stemming
    const QStringList tokens = QString::fromUtf8(utf8Out.data(), utf8Out.size())
                                       .simplified()
                                       .split(u' ', Qt::SkipEmptyParts);

    QStringList out;
    out.reserve(tokens.size());
    for (const QString &tok : tokens) {
        // Strip leading/trailing non-alphanumeric characters (punctuation,
        // quotes, etc.). Internal characters like apostrophes are kept so the
        // stemmers can handle contractions correctly.
        int start = 0, end = tok.size() - 1;
        while (start <= end && !tok[start].isLetterOrNumber()) {
            ++start;
        }
        while (end >= start && !tok[end].isLetterOrNumber()) {
            --end;
        }
        if (end < start) { // entirely punctuation — skip
            continue;
        }

        // Words shorter than 4 characters are not stemmed — stemmers are
        // unreliable on very short tokens and the trigram index handles them.
        const QString word = tok.mid(start, end - start + 1);
        if (word.length() < 4) {
            out.append(word);
            continue;
        }

        const QByteArray wordUtf8 = word.toUtf8();
        const auto *wBytes = reinterpret_cast<const sb_symbol *>(wordUtf8.constData());
        const int wLen = wordUtf8.size();

        // Try German stemmer first (TODO: Other languages?)
        const sb_symbol *stemmed = sb_stemmer_stem(m_stemmerDe, wBytes, wLen);
        int stemLen = sb_stemmer_length(m_stemmerDe);
        if (stemLen < wLen) {
            // German stemmer produced a shorter form — use it.
            out.append(QString::fromUtf8(reinterpret_cast<const char *>(stemmed), stemLen));
            continue;
        }

        // German stemmer left the word unchanged, try English
        stemmed = sb_stemmer_stem(m_stemmerEn, wBytes, wLen);
        stemLen = sb_stemmer_length(m_stemmerEn);

        out.append(QString::fromUtf8(reinterpret_cast<const char *>(stemmed), stemLen));
    }

    return out.join(u' ');
}
