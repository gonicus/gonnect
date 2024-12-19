#pragma once

#include <QObject>
#include <QTimer>
#include <pjsua2.hpp>

class RingTone : public QObject
{
    Q_OBJECT

public:
    /*!
     * \brief RingTone
     *
     * Creates a new RingTone object with the given sound specification.
     *
     * Example: RingTone(425,  // frequency1
     *                   0,    // frequency2
     *                   { {200,200}, {200,1000}, {200,200}, {200,1000}, {200,200}, {200,5000} }, //
     * intervals 4)    // loopIndex
     *
     * The frequencies are set to 425 Hz and 0 Hz (which means "off").
     *
     * It will then play this (read from top to down from left to right):
     *
     * +---+-----------------+-----------------+
     * | i | frequency1 [ms] | freuqency2 [ms] |
     * +---+-----------------+-----------------+
     * | 0 |             200 |             200 |
     * | 1 |             200 |            1000 |
     * | 2 |             200 |             200 |
     * | 3 |             200 |            1000 |
     * | 4 |             200 |             200 |
     * | 5 |             200 |            5000 |
     * +---+-----------------+-----------------+
     *
     * It will then repeat the last two lines (from loopIndex = 4) until stop() is called or a
     * timeout() occurs.
     *
     * \param frequency1 Frequency of tone 1 in Hz
     * \param frequency2 Frequency of tone 2 in Hz
     * \param intervals List of tuples how long (in ms) tone 1 and tone 2 are played
     * \param loopIndex Index in the intervals tuple list from which the sound shall be repeated;
     * must be a valid index of intervals; -1 means "no repeat" \param parent QObject parent
     */
    explicit RingTone(quint16 frequency1, quint16 frequency2,
                      QList<QPair<quint16, quint16>> intervals, qint8 loopIndex = -1,
                      QObject *parent = nullptr);
    virtual ~RingTone();

    /// Start the ring tone and automatically stop after having it played n times
    void start(quint8 times);
    void start();
    void stop();

signals:
    void ready();

private slots:
    void playNextTone();

private:
    bool m_isPlaying = false;
    pj::ToneGenerator m_toneGen;
    pj::AudioMedia &m_mediaSink;
    QTimer m_loopTimer;
    QTimer m_stopTimer;
    quint16 m_frequency1 = 0;
    quint16 m_frequency2 = 0;
    qint8 m_loopIndex = -1;
    qint16 m_repeatTimes = -1;
    quint8 m_currentIndex = 0;
    QList<QPair<quint16, quint16>> m_intervals;
};
