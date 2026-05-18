#include "TrustAnchors.h"

#include <p11-kit/p11-kit.h>
#include <p11-kit/pkcs11.h>

#include <QByteArray>
#include <QLoggingCategory>
#include <QVarLengthArray>

Q_LOGGING_CATEGORY(lcP11, "gonnect.p11kit")

namespace {

QList<QSslCertificate> certsFromSlot(CK_FUNCTION_LIST_PTR f, CK_SLOT_ID slot)
{
    QList<QSslCertificate> out;
    CK_SESSION_HANDLE sess = CK_INVALID_HANDLE;

    if (f->C_OpenSession(slot, CKF_SERIAL_SESSION, nullptr, nullptr, &sess) != CKR_OK) {
        return out;
    }

    CK_OBJECT_CLASS klass = CKO_CERTIFICATE;
    CK_CERTIFICATE_TYPE certType = CKC_X_509;
    CK_BBOOL trusted = CK_TRUE;

    CK_ATTRIBUTE tmpl[] = {
        { CKA_CLASS, &klass, sizeof(klass) },
        { CKA_CERTIFICATE_TYPE, &certType, sizeof(certType) },
        { CKA_TRUSTED, &trusted, sizeof(trusted) },
    };

    if (f->C_FindObjectsInit(sess, tmpl, sizeof(tmpl) / sizeof(tmpl[0])) != CKR_OK) {
        f->C_CloseSession(sess);
        return out;
    }

    for (;;) {
        CK_OBJECT_HANDLE handles[32];
        CK_ULONG count = 0;

        if (f->C_FindObjects(sess, handles, sizeof(handles) / sizeof(handles[0]), &count) != CKR_OK
            || count == 0) {
            break;
        }

        for (CK_ULONG i = 0; i < count; ++i) {
            CK_ATTRIBUTE attr = { CKA_VALUE, nullptr, 0 };
            if (f->C_GetAttributeValue(sess, handles[i], &attr, 1) != CKR_OK || attr.ulValueLen == 0
                || attr.ulValueLen == CK_UNAVAILABLE_INFORMATION) {
                continue;
            }

            QByteArray der(int(attr.ulValueLen), Qt::Uninitialized);
            attr.pValue = der.data();
            if (f->C_GetAttributeValue(sess, handles[i], &attr, 1) != CKR_OK) {
                continue;
            }

            QSslCertificate cert(der, QSsl::Der);
            if (!cert.isNull()) {
                out.append(cert);
            }
        }
    }

    f->C_FindObjectsFinal(sess);
    f->C_CloseSession(sess);

    return out;
}

QList<QSslCertificate> loadAnchorsFromP11Kit()
{
    QList<QSslCertificate> anchors;

    CK_FUNCTION_LIST_PTR_PTR mods = p11_kit_modules_load_and_initialize(P11_KIT_MODULE_TRUSTED);
    if (!mods) {
        qCWarning(lcP11, "p11-kit: %s", p11_kit_message());
        return anchors;
    }

    for (CK_FUNCTION_LIST_PTR_PTR m = mods; *m; ++m) {
        CK_ULONG nSlots = 0;
        if ((*m)->C_GetSlotList(CK_TRUE, nullptr, &nSlots) != CKR_OK || nSlots == 0) {
            continue;
        }

        QVarLengthArray<CK_SLOT_ID, 8> slots((int(nSlots)));
        if ((*m)->C_GetSlotList(CK_TRUE, slots.data(), &nSlots) != CKR_OK) {
            continue;
        }

        for (CK_ULONG i = 0; i < nSlots; ++i) {
            anchors += certsFromSlot(*m, slots[i]);
        }
    }

    p11_kit_modules_finalize_and_release(mods);

    return anchors;
}

} // namespace


TrustAnchors::TrustAnchors()
{
    m_qt = loadAnchorsFromP11Kit();

    QByteArray pem;
    for (const auto &c : std::as_const(m_qt)) {
        pem += c.toPem();
    }
    m_pem.assign(pem.constData(), size_t(pem.size()));

    qCInfo(lcP11, "p11-kit: %lld anchors loaded (%lld bytes PEM)", qlonglong(m_qt.size()),
           qlonglong(m_pem.size()));
}
