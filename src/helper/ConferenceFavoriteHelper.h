#pragma once

#include <QObject>
#include <QQmlEngine>

#include "GenericFavoriteHelper.h"

class ConferenceFavoriteHelper : public GenericFavoriteHelper
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString roomName MEMBER m_roomName NOTIFY roomNameChanged FINAL)

public:
    explicit ConferenceFavoriteHelper(QObject *parent = nullptr);

protected:
    virtual QString sanitizeContactString(const QString &value) override;

private:
    QString m_roomName;

Q_SIGNALS:
    void roomNameChanged();
};
