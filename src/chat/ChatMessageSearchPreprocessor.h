#pragma once

#include <QObject>
#include <QDateTime>
#include <QMutex>

#include <QtQml/qqml.h>
#include <QtQml/qqmlregistration.h>

#include <libstemmer.h>

#include <unicode/parseerr.h>
#include <unicode/translit.h>
#include <unicode/unistr.h>

class ChatMessageSearchPreprocessor : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(ChatMessageSearchPreprocessor)

public:
    Q_REQUIRED_RESULT static ChatMessageSearchPreprocessor &instance()
    {
        static ChatMessageSearchPreprocessor *_instance = nullptr;

        if (_instance == nullptr) {
            _instance = new ChatMessageSearchPreprocessor();
        }

        return *_instance;
    }

    ~ChatMessageSearchPreprocessor();

    QString process(const QString &text);

private:
    ChatMessageSearchPreprocessor(QObject *parent = nullptr);

    icu::Transliterator *m_transliterator = nullptr;

    sb_stemmer *m_stemmerDe = NULL;
    sb_stemmer *m_stemmerEn = NULL;

    QMutex m_preprocessorMutex;
};

class ChatMessageSearchPreprocessorWrapper
{
    Q_GADGET
    QML_FOREIGN(ChatMessageSearchPreprocessor)
    QML_NAMED_ELEMENT(ChatMessageSearchPreprocessor)
    QML_SINGLETON

public:
    static ChatMessageSearchPreprocessor *create(QQmlEngine *, QJSEngine *)
    {
        return &ChatMessageSearchPreprocessor::instance();
    }

private:
    ChatMessageSearchPreprocessorWrapper() = default;
};
