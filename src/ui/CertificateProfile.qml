pragma Singleton

import QtQuick
import QtWebEngine
import base

QtObject {
    id: control

    readonly property LoggingCategory _loggingCategory: LoggingCategory {
        id: category
        name: "gonnect.qml.CertificateProfile"
        defaultLogLevel: LoggingCategory.Warning
    }

    readonly property alias profile: internal.profile

    readonly property QtObject _internalObject: QtObject {
        id: internal

        property WebEngineProfile profile: null

        readonly property WebEngineProfilePrototype proto: WebEngineProfilePrototype {
            storageName: "CustomCertProfile"
            additionalTrustedCertificateFiles: CertHelper.caCertPaths()
        }

        Component.onCompleted: () => internal.createProfile()

        readonly property Connections certHelperConnections: Connections {
            target: CertHelper
            function onClientCertsInitializedChanged() {
                internal.createProfile()
            }
        }

        function createProfile() {
            if (!CertHelper.clientCertsInitialized) {
                CertHelper.initializeClientCerts()

            } else {
                const profile = internal.proto.instance()

                if (!profile) {
                    console.error(category, "CertificateProfile could not be loaded")
                    return
                }
                profile.offTheRecord = false

                const systemLocale = Qt.locale().name.replace("_", "-")
                const mainLang = systemLocale.split("-")[0]
                profile.httpAcceptLanguage = `${systemLocale},${mainLang};q=0.9,en;q=0.8`

                CertHelper.addClientCerts(profile)

                internal.profile = profile
            }
        }
    }
}
