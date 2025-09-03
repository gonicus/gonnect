#pragma once

#include <QObject>
#include "ExternalMediaManager.h"

class MPRISExternalMediaManager : public ExternalMediaManager
{
    Q_OBJECT

public:
    void pause() override;
    void resume() override;
    bool hasState() const override { return m_playerActiveTargets.size(); }

    explicit MPRISExternalMediaManager();
    ~MPRISExternalMediaManager();

private:
    QStringList getMprisTargets() const;

    QStringList m_playerActiveTargets;
};
