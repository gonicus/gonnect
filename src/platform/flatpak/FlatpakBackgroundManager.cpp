#include "FlatpakBackgroundManager.h"
#include "BackgroundPortal.h"
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcBackgroundManager, "gonnect.app.background")

BackgroundManager &BackgroundManager::instance()
{
    static BackgroundManager *_instance = nullptr;
    if (!_instance) {
        _instance = new FlatpakBackgroundManager;
    }
    return *_instance;
}

FlatpakBackgroundManager::FlatpakBackgroundManager() : BackgroundManager{}
{
    m_backgroundPortal = new BackgroundPortal(this);
}

void FlatpakBackgroundManager::request(bool autostart)
{
    m_backgroundPortal->RequestBackground(
            autostart, false, [this](uint code, const QVariantMap &response) {
                switch (code) {
                case 0:
                    m_autostart = response.value("autostart").toBool();
                    qCDebug(lcBackgroundManager)
                            << "autostart is set to" << response.value("autostart").toBool();
                    emit autostartChanged();

                    qCDebug(lcBackgroundManager)
                            << "background is set to" << response.value("background").toBool();
                    break;
                case 1:
                    qCWarning(lcBackgroundManager) << "autostart request was rejected by portal";
                    break;
                case 2:
                    qCWarning(lcBackgroundManager) << "autostart request to portal failed";
                    break;
                }
            });
}
