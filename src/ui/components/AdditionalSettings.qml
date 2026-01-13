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
        if (key in control.parameters) {
            return control.parameters[key]
        }
        return ""
    }

    function remove(key : string) {
        if (key in control.parameters) {
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
