// Replaces browser camera with GOnnect camera:
// frames fetched from gonnect-camera scheme and written into MediaStreamTrackGenerator

(() => {
    'use strict'

    const deviceId = 'gonnect-camera'
    const label = 'GOnnect'
    const frameUrl = 'gonnect-camera:frame'
    const frameFormats = [ 'I420', 'RGBX' ]
    const headerSize = 12
    const retryDelayMs = 500

    const mediaDevices = navigator.mediaDevices
    if (!mediaDevices) {
        return
    }

    if (!('MediaStreamTrackGenerator' in window) || !('VideoFrame' in window)) {
        console.error('GOnnect camera: MediaStreamTrackGenerator or VideoFrame not available')
        return
    }

    const enumerateDevices = mediaDevices.enumerateDevices.bind(mediaDevices)
    const getUserMedia = mediaDevices.getUserMedia.bind(mediaDevices)
    const camera = { deviceId, groupId: deviceId, kind: 'videoinput', label }

    mediaDevices.enumerateDevices = async () => {
        const devices = await enumerateDevices()
        return [ ...devices.filter(devices => devices.kind !== 'videoinput'), camera ]
    }

    mediaDevices.getUserMedia = async (constraints = {}) => {
        if (!constraints.video) {
            return getUserMedia(constraints)
        }

        const stream = constraints.audio 
                       ? await getUserMedia({ audio: constraints.audio })
                       : new MediaStream()
        stream.addTrack(createCameraTrack())
        return stream
    }

    function createCameraTrack() {
        const track = new MediaStreamTrackGenerator({ kind: 'video' })
        const abortController = new AbortController()
        const settings = { deviceId, groupId: deviceId, width: 0, height: 0 }

        Object.defineProperty(track, 'label', { value: label })
        track.getSettings = () => ({ ...settings })
        track.applyConstraints = async () => {}

        const stop = track.stop.bind(track)
        track.stop = () => {
            abortController.abort()
            stop()
        }

        pumpFrames(track, settings, abortController.signal)
        return track
    }

    async function pumpFrames(track, settings, signal) {
        const writer = track.writable.getWriter()

        while (!signal.aborted && track.readyState === 'live') {
            try {
                const response = await fetch(frameUrl, { signal, cache: 'no-store' })
                const frame = toVideoFrame(await response.arrayBuffer())

                settings.width = frame.codedWidth
                settings.height = frame.codedHeight

                await writer.write(frame)
            } catch (error) {
                if (signal.aborted) {
                    break
                }

                console.warn('GOnnect camera: cannot fetch frame:', error.message)
                await new Promise(resolve => setTimeout(resolve, retryDelayMs))
            }
        }

        writer.close().catch(() => {})
    }

    function toVideoFrame(buffer) {
        const header = new DataView(buffer, 0, headerSize)

        return new VideoFrame(new Uint8Array(buffer, headerSize), {
            format: frameFormats[header.getUint32(0, true)],
            codedWidth: header.getUint32(4, true),
            codedHeight: header.getUint32(8, true),
            timestamp: Math.round(performance.now() * 1000)
        })
    }
})()

