#pragma once

#include <QString>
#include <QList>

struct WebDavPropertyValue
{
    QString text;
    QList<std::pair<QString, QString>> childElements;
};
