#pragma once

#include <QObject>
#include <QDateTime>
#include <QMutexLocker>

#include <libstemmer.h>

#include <unicode/parseerr.h>
#include <unicode/translit.h>
#include <unicode/unistr.h>

class ChatMessageSearchPreprocessor : public QObject
{
    Q_OBJECT

public:
    ChatMessageSearchPreprocessor(QObject *parent = nullptr);
    ~ChatMessageSearchPreprocessor();

    QString process(const QString &text);

private:
    icu::Transliterator *m_transliterator = nullptr;

    sb_stemmer *m_stemmerDe = NULL;
    sb_stemmer *m_stemmerEn = NULL;

    QMutex m_preprocessorMutex;
};
