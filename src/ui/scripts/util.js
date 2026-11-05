.pragma library

function clamp(x, min, max) {
    return Math.min(Math.max(x, min), max)
}

function snapToDevicePixel(value, devicePixelRatio) {
    return Math.round(value * devicePixelRatio) / devicePixelRatio
}

function snapToPixelGrid(value, quantum) {
    return Math.round(value / quantum) * quantum
}
