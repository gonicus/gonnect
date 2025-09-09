#include <QLoggingCategory>
#include <QRegularExpression>

#include "TogglerManager.h"

Q_LOGGING_CATEGORY(lcTogglerManager, "gonnect.sip.toggler")

TogglerManager::TogglerManager(QObject *parent) : QObject(parent) { }

void TogglerManager::initialize()
{
    static QRegularExpression isTogglerGroup = QRegularExpression("^toggler_[0-9]+$");

    // Look for accountN groups
    QStringList groups = m_settings.childGroups();
    for (auto &group : std::as_const(groups)) {

        if (isTogglerGroup.match(group).hasMatch()) {
            auto toggler = new Toggler(group, this);
            if (toggler->initialize()) {
                qCInfo(lcTogglerManager) << "created toggler" << group;
                m_toggler.push_back(toggler);

                connect(toggler, &Toggler::activeChanged, this, [this, toggler]() {
                    Q_EMIT togglerActiveChanged(toggler, toggler->isActive());
                });
                connect(toggler, &Toggler::busyChanged, this,
                        [this, toggler]() { Q_EMIT togglerBusyChanged(toggler, toggler->isBusy()); });

            } else {
                qCCritical(lcTogglerManager)
                        << "skipped" << group << "due to initialization errors";
            }
        }
    }

    Q_EMIT togglerChanged();
}

Toggler *TogglerManager::getToggler(const QString &id)
{
    for (Toggler *tg : std::as_const(m_toggler)) {
        if (tg->id() == id) {
            return tg;
        }
    }

    return nullptr;
}

void TogglerManager::toggleToggler(const QString &id)
{
    if (auto toggler = getToggler(id)) {
        toggler->setActive(!toggler->isActive());
    }
}
