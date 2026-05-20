#pragma once
#include <QList>
#include <QSslCertificate>
#include <string>

class TrustAnchors
{
public:
    Q_REQUIRED_RESULT static TrustAnchors &instance()
    {
        static TrustAnchors *_instance = nullptr;

        if (_instance == nullptr) {
            _instance = new TrustAnchors();
        }

        return *_instance;
    }

    const QList<QSslCertificate> &qtCerts() const { return m_qt; }
    const std::string &pemBundle() const { return m_pem; }

private:
    TrustAnchors();
    QList<QSslCertificate> m_qt;
    std::string m_pem;
};
