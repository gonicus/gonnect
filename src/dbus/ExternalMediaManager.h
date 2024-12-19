#pragma once

#include <QObject>

class ExternalMediaManager : public QObject
{
    Q_OBJECT

public:
    Q_REQUIRED_RESULT static ExternalMediaManager &instance()
    {
        static ExternalMediaManager *_instance = nullptr;

        if (_instance == nullptr) {
            _instance = new ExternalMediaManager();
        }

        return *_instance;
    }

    void pause();
    void resume();
    bool hasState() const { return m_playerActiveTargets.size(); }

    ~ExternalMediaManager();

private:
    explicit ExternalMediaManager(QObject *parent = nullptr);
    QStringList getMprisTargets() const;

    QStringList m_playerActiveTargets;
};
