import QtQuick as QQ
import base

QQ.TextEdit {
    font.pixelSize: Theme.fontPixelSize
    color: control.enabled ? Theme.primaryTextColor : Theme.secondaryTextColor
}
