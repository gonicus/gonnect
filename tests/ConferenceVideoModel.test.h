#pragma once

#include <QObject>

class ConferenceVideoModelTest : public QObject
{
    Q_OBJECT

private slots:
    void addsAndRemovesSources();
    void addSourceIsIdempotent();
    void reportsMuteChange();
    void clearsAllSources();
    void keepsFramePumpAliveForDecoder();
    void outputFeedsSinkAndReportsHeight();
};
