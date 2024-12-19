#pragma once

#include <QObject>

#include <pjsua2/media.hpp>

class DtmfGenerator : public QObject
{
    Q_OBJECT

public:
    explicit DtmfGenerator(QObject *parent = nullptr);

    /// Plays the given key (single digit, *, #, A-D)
    void playDtmf(const QChar &key);

private:
    pj::ToneGenerator m_toneGen;
    pj::AudioMedia &m_mediaSink;
};
