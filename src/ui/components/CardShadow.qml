pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Effects
import base

MultiEffect {
    blurEnabled: false

    shadowEnabled: true
    shadowColor: Theme.shadowColor
    shadowBlur: 0.5
    shadowOpacity: 1.0
    shadowScale: 1.0
}
