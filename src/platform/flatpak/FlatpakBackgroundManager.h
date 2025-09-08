#pragma once
#include <QObject>
#include "../BackgroundManager.h"

class BackgroundPortal;

class FlatpakBackgroundManager : public BackgroundManager
{
    Q_OBJECT

public:
    explicit FlatpakBackgroundManager();

    void request(bool autostart) override;
    bool autostart() override { return m_autostart; }

private:
    BackgroundPortal *m_backgroundPortal = nullptr;

    bool m_autostart = false;
};
