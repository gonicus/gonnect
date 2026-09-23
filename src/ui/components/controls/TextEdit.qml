import QtQuick as QQ
import base

QQ.TextEdit {
    font.pixelSize: Theme.fontSizeNormal
    color: control.enabled ? Theme.primaryTextColor : Theme.secondaryTextColor
}
