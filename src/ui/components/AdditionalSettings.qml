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

    function get(key : string) {
        return control.parameters[key]
    }

    function remove(key : string) {
        if (key in control.parameters) {
            delete control.parameters[key]

            control.parametersUpdated()
        }
    }

    function entries() {
        var keys = Object.keys(control.parameters)
        return keys.join(",")
    }

    function data() {
        return control.parameters
    }
}
