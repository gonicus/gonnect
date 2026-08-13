#pragma once

#include <QObject>
#include <QSet>
#include "ExternalMediaManager.h"
#include <winrt/Windows.Foundation.h>

class SmtcThread;

class SmtcExternalMediaManager : public ExternalMediaManager
{
    Q_OBJECT

public:
    void pause() override;
    void resume() override;
    bool hasState() const override { return m_didPause; }

    explicit SmtcExternalMediaManager();
    ~SmtcExternalMediaManager();

private:
    SmtcThread *m_thread = nullptr;
    bool m_didPause = false;
};
