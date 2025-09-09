#include "PhoneNumberUtil.h"
#include "ResponseLoader.h"
#include "ResponseItem.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QDebug>
#include <QLoggingCategory>

#include <QDirIterator>

Q_LOGGING_CATEGORY(lcResponseLoader, "gonnect.app.ResponseLoader")

ResponseLoader::ResponseLoader(const QString &sourceData, QObject *parent)
    : QObject{ parent }, m_sourceData(sourceData)
{
    assrt(!m_sourceData.isEmpty(), "Source data is empty");
}

void ResponseLoader::assrt(bool cond, QString err)
{
    if (!cond) {
        qCWarning(lcResponseLoader) << "Assert error:" << err;
        Q_EMIT error(err);
    }
}

QList<ResponseItem *> ResponseLoader::loadResponse()
{
    QLocale locale = QLocale::system();
    QList<ResponseItem *> response;

    QJsonParseError jsonErr;
    m_dataDoc = QJsonDocument::fromJson(m_sourceData.toUtf8(), &jsonErr);
    if (jsonErr.error) {
        Q_EMIT error(jsonErr.errorString());
        return response;
    } else if (m_dataDoc.isNull() || !m_dataDoc.isArray()) {
        Q_EMIT error("Invalid JSON format in response");
        return response;
    }

    assrt(m_dataDoc.isArray(), "elements on main level array");

    const QJsonArray arr = m_dataDoc.array();
    for (const auto el : arr) {
        assrt(el.isObject(), "elements on main level object");
        const auto mObj = el.toObject();

        assrt(mObj.contains("category"), "main element must contain key 'category'");
        assrt(mObj.contains("title"), "main element must contain key 'title'");
        assrt(mObj.contains("items"), "main element must contain key 'items'");

        assrt(mObj.value("items").isArray(), "elements on 'item' array");
        const auto its = mObj.value("items").toArray();

        ResponseItem *entry = new ResponseItem();
        entry->category = mObj.value("category").toString();
        entry->title = mObj.value("title").toString();

        for (const auto it : its) {
            assrt(it.isObject(), "elements on 'item' object");
            const auto itObj = it.toObject();

            ResponseItem::Item *item = new ResponseItem::Item();

            assrt(itObj.contains("title"), "'items' must contain key 'title'");
            assrt(itObj.contains("url"), "'items' must contain key 'url'");
            item->title =
                    QString("<a href=\"%1\">%2</a>")
                            .arg(itObj.value("url").toString(), itObj.value("title").toString());

            if (itObj.contains("description")) {
                item->description = itObj.value("description").toString();
            } else {
                item->description = "";
            }

            // item.created
            if (itObj.contains("created")) {
                ResponseItem::Reference *created = new ResponseItem::Reference();

                assrt(itObj.value("created").isObject(), "elements on 'created' object");
                const auto cObj = itObj.value("created").toObject();
                assrt(cObj.contains("name"), "'created' must contain key 'name'");
                assrt(cObj.contains("mail"), "'created' must contain key 'mail'");
                assrt(cObj.contains("sipURI"), "'created' must contain key 'sipURI'");
                assrt(cObj.contains("date"), "'created' must contain key 'date'");

                created->name = cObj.value("name").toString();
                created->mail = QString("✉️ <a href=\"mailto:%1\">%1</a>")
                                        .arg(cObj.value("mail").toString());
                created->sipUri = QString("📞 <a href=\"tel:%1\">%1</a>")
                                          .arg(PhoneNumberUtil::numberFromSipUrl(
                                                  cObj.value("sipURI").toString()));

                QString date = cObj.value("date").toString();
                QDateTime iso = QDateTime::fromString(date, Qt::ISODate);
                if (iso.isValid()) {
                    created->date = locale.toString(iso, QLocale::ShortFormat);
                } else {
                    created->date = date; // Some other format is used here
                }

                item->created = created;
            } else {
                item->created = nullptr;
            }

            // item.latestActivity
            if (itObj.contains("latestActivity")) {
                ResponseItem::Reference *latestActivity = new ResponseItem::Reference();

                assrt(itObj.value("latestActivity").isObject(),
                      "elements on 'latestActivity' object");
                const auto laObj = itObj.value("latestActivity").toObject();
                assrt(laObj.contains("name"), "'latestActivity' must contain key 'name'");
                assrt(laObj.contains("mail"), "'latestActivity' must contain key 'mail'");
                assrt(laObj.contains("sipURI"), "'latestActivity' must contain key 'sipURI'");
                assrt(laObj.contains("date"), "'latestActivity' must contain key 'date'");

                latestActivity->name = laObj.value("name").toString();
                latestActivity->mail = QString("✉️ <a href=\"mailto:%1\">%1</a>")
                                               .arg(laObj.value("mail").toString());
                latestActivity->sipUri = QString("📞 <a href=\"tel:%1\">%1</a>")
                                                 .arg(PhoneNumberUtil::numberFromSipUrl(
                                                         laObj.value("sipURI").toString()));

                QString date = laObj.value("date").toString();
                QDateTime iso = QDateTime::fromString(date, Qt::ISODate);
                if (iso.isValid()) {
                    latestActivity->date = locale.toString(iso, QLocale::ShortFormat);
                } else {
                    latestActivity->date = date; // Some other format is used here
                }

                item->latestActivity = latestActivity;
            } else {
                item->latestActivity = nullptr;
            }

            entry->items.append(item);
        }

        response.append(entry);
    }

    qCDebug(lcResponseLoader) << "JSON response entries:" << response.count();

    Q_EMIT finished();
    return response;
}
