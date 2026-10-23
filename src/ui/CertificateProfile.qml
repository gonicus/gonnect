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

        Component.onCompleted: () => {

                                   // Must be done onCompleted and not in binding because not everything would be loaded completely
                                   const profile = internal.proto.instance()

                                   if (!profile) {
                                       console.error(category, "CertificateProfile could not be loaded")
                                       return
                                   }
                                   profile.offTheRecord = false

                                   const systemLocale = Qt.locale().name.replace("_", "-")
                                   const mainLang = systemLocale.split("-")[0]
                                   profile.httpAcceptLanguage = `${systemLocale},${mainLang};q=0.9,en;q=0.8`

                                   internal.profile = profile
                               }
    }
}
