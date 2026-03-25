pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material
import QtQuick.Controls.impl
import base

Popup {
    id: volumePopup
    transformOrigin: Item.Top
    enter: null  // Transitions seem to cause that the popup is not visible sometimes...
    exit: null
    closePolicy: Popup.CloseOnPressOutsideParent

    Material.theme: Theme.isDarkMode ? Material.Dark : Material.Light

    ColumnLayout {
        anchors {
            left: parent.left
            right: parent.right
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Image {
                source: Icons.brightnessHigh
                sourceSize.width: 22
                sourceSize.height: 22
            }

            Slider {
                id: brightnessSlider

                from: 0
                to: 100
                stepSize: 1
                value: BusylightDeviceManager.streamingLightBrightness

                Layout.preferredWidth: brightnessSlider.implicitWidth
                Layout.preferredHeight: brightnessSlider.implicitHeight

                onMoved: () => BusylightDeviceManager.setStreamingLightBrightness(brightnessSlider.value)

                Accessible.role: Accessible.Slider
                Accessible.name: qsTr("Adjust the brightness")
                Accessible.focusable: true
                Accessible.onIncreaseAction: () => {
                    if (brightnessSlider.value < brightnessSlider.to) {
                        brightnessSlider.value += brightnessSlider.stepSize
                    }
                }
                Accessible.onDecreaseAction: () => {
                    if (brightnessSlider.value > brightnessSlider.from) {
                        brightnessSlider.value -= brightnessSlider.stepSize
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Image {
                source: Icons.colorManagement
                sourceSize.width: 22
                sourceSize.height: 22
            }

            Slider {
                id: colorTemperatureSlider
                Layout.preferredWidth: colorTemperatureSlider.implicitWidth
                Layout.preferredHeight: colorTemperatureSlider.implicitHeight

                onMoved: () => console.log(colorTemperatureSlider.value)

                Accessible.role: Accessible.Slider
                Accessible.name: qsTr("Adjust the color temperature")
                Accessible.focusable: true
                Accessible.onIncreaseAction: () => {
                    if (colorTemperatureSlider.value < colorTemperatureSlider.to) {
                        colorTemperatureSlider.value += colorTemperatureSlider.stepSize
                    }
                }
                Accessible.onDecreaseAction: () => {
                    if (colorTemperatureSlider.value > colorTemperatureSlider.from) {
                        colorTemperatureSlider.value -= colorTemperatureSlider.stepSize
                    }
                }
            }
        }
    }
}
