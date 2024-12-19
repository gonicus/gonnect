#pragma once

#include <QObject>

class RingTone;

class RingToneFactory : public QObject
{
    Q_OBJECT

public:
    static RingToneFactory &instance()
    {
        static RingToneFactory *_instance = nullptr;
        if (!_instance) {
            _instance = new RingToneFactory;
        }
        return *_instance;
    }

    RingTone *ringingTone();
    RingTone *busyTone();
    RingTone *congestionTone();
    RingTone *zipTone();
    RingTone *endTone();

private:
    explicit RingToneFactory(QObject *parent = nullptr);
    void loadConfig();
    RingTone *createRingToneObjectFromConfigString(const QString &configString);

    bool m_configLoaded = false;

    RingTone *m_ringingTone = nullptr;
    RingTone *m_busyTone = nullptr;
    RingTone *m_congestionTone = nullptr;
    RingTone *m_zipTone = nullptr;
    RingTone *m_endTone = nullptr;
};
