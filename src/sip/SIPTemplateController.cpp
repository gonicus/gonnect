#include "SIPTemplateController.h"
#include "SIPTemplates.h"
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcSIPTemplateController, "gonnect.sip.templates.controller")

SIPTemplateController::SIPTemplateController(QObject *parent) : QObject{ parent } { }

QVariantMap SIPTemplateController::createConfig(const QString &templateId,
                                                const QVariantMap &values) const
{
    const auto templ = SIPTemplates::instance().templateById(templateId);
    Q_CHECK_PTR(templ);
    return templ->save(values);
}
