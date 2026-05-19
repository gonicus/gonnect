#pragma once

#include <QQuickImageProvider>

class PersonCoinProvider : public QQuickImageProvider
{
public:
    PersonCoinProvider();

    virtual QImage requestImage(const QString &id, QSize *size,
                                const QSize &requestedSize) override;

    QString makePath(const QString &id, const int size) const;
};
