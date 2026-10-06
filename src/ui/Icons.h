#pragma once

#include <QQmlPropertyMap>
#include <QQmlEngine>

class Icons : public QQmlPropertyMap
{
    Q_OBJECT
    QML_SINGLETON
    QML_NAMED_ELEMENT(Icons)

public:
    explicit Icons(QObject *parent = nullptr);

private:
    void updateIconPaths();
};
