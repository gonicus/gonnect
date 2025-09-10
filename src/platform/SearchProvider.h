#pragma once

#include <QObject>
#include <QHash>
#include <QtQml/qqml.h>
#include <QtQml/qqmlregistration.h>

class SearchProvider : public QObject
{
    Q_OBJECT

public:
    Q_REQUIRED_RESULT static SearchProvider &instance();
    explicit SearchProvider(QObject *parent = nullptr) : QObject(parent) { }
    ~SearchProvider() = default;

Q_SIGNALS:
    void activateSearch(QString query);
};

class SearchProviderWrapper
{
    Q_GADGET
    QML_FOREIGN(SearchProvider)
    QML_NAMED_ELEMENT(SearchProvider)
    QML_SINGLETON

public:
    static SearchProvider *create(QQmlEngine *, QJSEngine *) { return &SearchProvider::instance(); }

private:
    SearchProviderWrapper() = default;
};
