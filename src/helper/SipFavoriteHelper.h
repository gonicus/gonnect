#pragma once

#include <QQmlEngine>

#include "GenericFavoriteHelper.h"

class SipFavoriteHelper : public GenericFavoriteHelper
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString sipAddress MEMBER m_sipAddress NOTIFY sipAddressChanged FINAL)

public:
    SipFavoriteHelper(QObject *parent = nullptr);

protected:
    virtual QString sanitizeContactString(const QString &value) override;

private:
    QString m_sipAddress;

Q_SIGNALS:
    void sipAddressChanged();
};
