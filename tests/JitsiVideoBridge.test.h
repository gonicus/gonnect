#pragma once

#include "ConferenceVideoModel.h"
#include "JitsiVideoBridge.h"

#include <QObject>

#include <memory>

class JitsiVideoBridgeTest : public QObject
{
    Q_OBJECT

private slots:

    void init();
    void cleanup();

    void rejectsInvalidToken();
    void rejectsForeignOrigin();
    void decodesTrackIntoSource();
    void requestsKeyFrameForInterFrame();

private:
    std::unique_ptr<ConferenceVideoModel> m_model;
    std::unique_ptr<JitsiVideoBridge> m_bridge;
};
