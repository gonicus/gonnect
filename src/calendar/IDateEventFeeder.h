#pragma once

#include <QUrl>

class IDateEventFeeder
{

public:
    virtual void init() = 0;
    virtual QUrl networkCheckURL() const { return QUrl(); }
};
