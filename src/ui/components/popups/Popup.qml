import QtQuick
import QtQuick.Controls.Material as M
import QtQuick.Templates as T
import base as Base

M.Popup {
    focus: true

    T.Overlay.modal: Base.PopupBackgroundOverlay {}
}
