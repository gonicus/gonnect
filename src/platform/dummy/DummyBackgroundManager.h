#pragma once
#include <QObject>
#include "../BackgroundManager.h"

class DummyBackgroundManager : public BackgroundManager
{
    Q_OBJECT

public:
    explicit DummyBackgroundManager() : BackgroundManager() { }

    void request(bool autostart) override { Q_UNUSED(autostart); }
    bool autostart() override { return false; }
};
