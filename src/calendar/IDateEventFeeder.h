#pragma once

#include <QUrl>

class IDateEventFeeder
{

public:
    virtual void init() = 0;
    virtual QUrl networkCheckURL() const { return QUrl(); }

    virtual bool isInitialized() const { return m_isInitialized; }

protected:
    bool m_isInitialized = false;
};

/*
void myCriticalFunction() {
    // Versucht, die eine verfügbare Ressource zu belegen
    if (runOnceSemaphore.tryAcquire(1)) {
        // Dieser Block wird NUR beim ersten erfolgreichen Aufruf ausgeführt
        qDebug() << "Funktion wird zum ersten (und einzigen) Mal ausgeführt.";
        // WICHTIG: Um eine EINMALIGE Ausführung über die gesamte Lebensdauer
        // zu garantieren, rufen wir hier KEIN release() auf.
    } else {
        // Alle weiteren Aufrufe landen hier, da keine Ressource mehr frei ist
        qDebug() << "Funktion wurde bereits ausgeführt. Überspringe...";
    }
}
*/
