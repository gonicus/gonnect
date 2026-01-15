pragma ComponentBehavior: Bound

import QtQuick
import base

Item {
    id: control

    property var parameters: ({})

    signal parametersUpdated()

    function set(key : string, value : variant) {
        control.parameters[key] = value

        control.parametersUpdated()
    }

    function get(key : string): variant {
        if (control.parameters.hasOwnProperty(key)) {
            return control.parameters[key]
        }
        return ""
    }

    function remove(key : string) {
        if (control.parameters.hasOwnProperty(key)) {
            delete control.parameters[key]

            control.parametersUpdated()
        }
    }

    function entries() {
        return Object.keys(control.parameters)
    }

    function data() {
        return control.parameters
    }
}
