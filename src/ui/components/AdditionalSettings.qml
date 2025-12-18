pragma ComponentBehavior: Bound

import QtQuick
import base

Item {
    id: control

    property var parameters: ({})

    signal parametersUpdated()

    function set(key : string, value : string) {
        control.parameters[key] = value

        control.parametersUpdated()
    }

    function get(key : string): variant {
        return control.parameters[key]
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
