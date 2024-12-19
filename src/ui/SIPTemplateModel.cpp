#include "SIPTemplateModel.h"
#include "SIPTemplates.h"

SIPTemplateModel::SIPTemplateModel(QObject *parent) : QAbstractListModel{ parent }
{
    connect(this, &SIPTemplateModel::templateIdChanged, this,
            &SIPTemplateModel::onTemplateIdChanged);
}

QHash<int, QByteArray> SIPTemplateModel::roleNames() const
{
    return {
        { static_cast<int>(Roles::Name), "name" },
        { static_cast<int>(Roles::Description), "description" },
        { static_cast<int>(Roles::Target), "target" },
        { static_cast<int>(Roles::Type), "type" },
        { static_cast<int>(Roles::Regex), "regex" },
        { static_cast<int>(Roles::Required), "required" },
        { static_cast<int>(Roles::MimeType), "mimeType" },
        { static_cast<int>(Roles::FileSuffixes), "fileSuffixes" },
    };
}

int SIPTemplateModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_template ? m_template->fields().size() : 0;
}

QVariant SIPTemplateModel::data(const QModelIndex &index, int role) const
{
    const auto field = m_template->fields().at(index.row());

    switch (role) {
    case static_cast<int>(Roles::Description):
        return field->description;
    case static_cast<int>(Roles::Target):
        return field->target;
    case static_cast<int>(Roles::Type):
        return static_cast<int>(field->type);
    case static_cast<int>(Roles::Regex):
        return field->regex;
    case static_cast<int>(Roles::Required):
        return field->required;
    case static_cast<int>(Roles::MimeType):
        return field->mimeType;
    case static_cast<int>(Roles::FileSuffixes):
        return SIPTemplates::suffixesForMimeType(field->mimeType);
    case static_cast<int>(Roles::Name):
    default:
        return field->name;
    }
}

void SIPTemplateModel::onTemplateIdChanged()
{
    beginResetModel();

    if (m_template) {
        m_template->disconnect(this);
        m_template = nullptr;
    }

    if (!m_templateId.isEmpty()) {

        const auto templates = SIPTemplates::instance().templates();
        for (auto templ : templates) {
            if (templ->id() == m_templateId) {
                m_template = templ;
                break;
            }
        }
    }

    connect(m_template, &QObject::destroyed, this, [this]() {
        beginResetModel();
        m_template = nullptr;
        endResetModel();
    });

    endResetModel();
}
