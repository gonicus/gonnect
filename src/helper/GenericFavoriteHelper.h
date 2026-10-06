#pragma once

#include <QObject>

#include "NumberStats.h"

class GenericFavoriteHelper : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool isFavorite READ isFavorite NOTIFY isFavoriteChanged FINAL)

public:
    Q_INVOKABLE void toggleFavorite() const;

    bool isFavorite() const { return m_isFavorite; }

protected:
    explicit GenericFavoriteHelper(NumberStats::ContactType contactType, QObject *parent = nullptr);

    void setContactString(const QString &value);
    virtual QString sanitizeContactString(const QString &value) = 0;

private:
    NumberStats::ContactType m_contactType;
    bool m_isFavorite = false;
    QString m_contactString;

protected Q_SLOTS:
    void updateIsFavorite();

Q_SIGNALS:
    void isFavoriteChanged();
};
