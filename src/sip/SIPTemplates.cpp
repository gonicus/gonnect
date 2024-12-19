#include <QDirIterator>
#include "SIPTemplates.h"

QStringList SIPTemplates::suffixesForMimeType(const QString &mimeType)
{
    static const QHash<QString, QStringList> endings = {
        { "application/x-pem-file", { "pem" } },
        { "application/x-x509-ca-cert", { "crt" } },
    };

    return endings.value(mimeType);
}

SIPTemplate *SIPTemplates::templateById(const QString &id) const
{
    for (auto templ : std::as_const(m_templates)) {
        if (templ->id() == id) {
            return templ;
        }
    }
    return nullptr;
}

SIPTemplates::SIPTemplates(QObject *parent) : QObject(parent)
{
    QDirIterator it(":/templates", QDirIterator::Subdirectories);

    while (it.hasNext()) {
        SIPTemplate *templ = new SIPTemplate(it.next(), this);
        if (templ->isValid()) {
            m_templates.push_back(templ);
        } else {
            delete templ;
        }
    }
}
