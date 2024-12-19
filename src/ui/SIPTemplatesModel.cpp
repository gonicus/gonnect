#include "SIPTemplatesModel.h"
#include "SIPTemplates.h"
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcSIPTemplatesModels, "gonnect.sip.templates.model")

SIPTemplatesModel::SIPTemplatesModel(QObject *parent) : QAbstractListModel{ parent } { }

bool SIPTemplatesModel::hasFields(const QString &id) const
{
    const auto &templates = SIPTemplates::instance().templates();
    for (const auto templ : templates) {
        if (templ->id() == id) {
            return templ->fields().size();
        }
    }
    qCCritical(lcSIPTemplatesModels) << "Cannot find template with id" << id << "- ignoring";
    return false;
}

QHash<int, QByteArray> SIPTemplatesModel::roleNames() const
{
    return {
        { static_cast<int>(Roles::Id), "id" },
        { static_cast<int>(Roles::Name), "name" },
    };
}

int SIPTemplatesModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return SIPTemplates::instance().templates().size();
}

QVariant SIPTemplatesModel::data(const QModelIndex &index, int role) const
{
    const auto templ = q_check_ptr(SIPTemplates::instance().templates().at(index.row()));

    switch (role) {
    case static_cast<int>(Roles::Id):
        return templ->id();
    case static_cast<int>(Roles::Name):
    default:
        return templ->name();
    }
}
