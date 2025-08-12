#pragma once

#include <QUrl>

class IDateEventFeeder
{

public:
    virtual void process() = 0;
    virtual QUrl networkCheckURL() const { return QUrl(); }
};
