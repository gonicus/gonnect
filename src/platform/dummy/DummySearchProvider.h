#pragma once

#include <QObject>
#include "SearchProvider.h"

class DummyDesktopSearchProvider : public SearchProvider
{
    Q_OBJECT

public:
    explicit DummyDesktopSearchProvider(QObject *parent = nullptr) : SearchProvider(parent) { }
    ~DummyDesktopSearchProvider() = default;
};
