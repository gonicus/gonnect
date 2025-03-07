#include "ErrorBus.h"
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcErrorBus, "gonnect.app.error")

ErrorBus::ErrorBus(QObject *parent) : QObject{ parent } { }

void ErrorBus::addError(const QString &message)
{
    qCCritical(lcErrorBus) << "ERROR:" << message;
    emit error(message);
}

void ErrorBus::addFatalError(const QString &message)
{
    qCCritical(lcErrorBus) << "FATAL ERROR:" << message;
    emit fatalError(message);
}
