#pragma once
#include <QObject>

class ResponseItem : public QObject
{
    Q_OBJECT
public:
    struct Reference
    {
        QString name;
        QString mail;
        QString sipUri;
        QString date;
    };

    struct Item
    {
        QString title;
        QString url;
        QString description;
        Reference *created = nullptr;
        Reference *latestActivity = nullptr;

        ~Item()
        {
            if (created) {
                delete created;
            }

            if (latestActivity) {
                delete latestActivity;
            }
        }
    };

    ResponseItem(QObject *parent = nullptr);
    ResponseItem(const QString &category, const QString &title, const QList<Item *> &items,
                 QObject *parent);
    ~ResponseItem();

    QString category;
    QString title;
    QList<Item *> items;
};
