#pragma once

#include <QObject>
#include "ExternalMediaManager.h"

class DummyExternalMediaManager : public ExternalMediaManager
{
    Q_OBJECT

public:
    void pause() override { }
    void resume() override { }
    bool hasState() const override { return false; }

    explicit DummyExternalMediaManager() : ExternalMediaManager() { }
    ~DummyExternalMediaManager() = default;
};
