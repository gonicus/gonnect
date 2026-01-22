#pragma once
#include <QObject>
#include <QColor>
#include "../ThemeManager.h"

class DummyThemeManager : public ThemeManager
{
    Q_OBJECT

public:
    explicit DummyThemeManager();

    QColor accentColor() const override { return QColor("blue"); }
    bool highContrast() const override { return false; }

    void shutdown() override { }
};
