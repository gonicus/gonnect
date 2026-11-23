// Replaces browser camera with GOnnect camera:
// worker fetches frames from gonnect-camera scheme and writes into MediaStreamTrackGenerator

(() => {
    'use strict'

    const deviceId = 'gonnect-camera'
    const label = 'GOnnect'
    const defaultSize = { width: 1280, height: 720 }

    const mediaDevices = navigator.mediaDevices
    if (!mediaDevices) {
        return
    }

    if (!('MediaStreamTrackGenerator' in window) || !('VideoFrame' in window)) {
        console.error('GOnnect camera: MediaStreamTrackGenerator or VideoFrame not available')
        return
    }

    const workerUrl = createWorkerUrl()
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
        stream.addTrack(createCameraTrack(constraints.video))
        return stream
    }

    function createCameraTrack(videoConstraints) {
        const track = new MediaStreamTrackGenerator({ kind: 'video' })

        let trackConstraints = typeof videoConstraints === 'object' ? { ...videoConstraints } : {}

        const settings = { 
            deviceId, 
            groupId: deviceId, 
            width: requestedSize(trackConstraints.width) || defaultSize.width, 
            height: requestedSize(trackConstraints.height) || defaultSize.height 
        }

        Object.defineProperty(track, 'label', { value: label })
        track.getSettings = () => ({ ...settings })
        track.getConstraints = () => ({ ...trackConstraints })
        track.applyConstraints = async (constraints = {}) => {
            trackConstraints = { ...constraints }
        }

        // Receiving/copying frames block Jitsi's main thread
        const worker = new Worker(workerUrl)
        worker.onmessage = event => {
            const { type, width, height, message } = event.data
            if (type === 'size') {
                settings.width = width
                settings.height = height
            } else if (type === 'failed') {
                console.warn('GOnnect camera: frames not available:', message)
                track.stop()
            }
        }
        worker.onerror = event => {
            console.error('GOnnect camera: worker failed', event.message)
            track.stop()
        }

        const stop = track.stop.bind(track)
        track.stop = () => {
            worker.terminate()
            stop()
        }

        const writable = track.writable
        worker.postMessage({ writable }, [ writable ])
        return track
    }

    function requestedSize(constraint) {
        if (typeof constraint === 'number') {
            return constraint
        }
        return constraint?.exact ?? constraint?.ideal ?? constraint?.max ?? 0
    }

    function createWorkerUrl() {
        const settings = {
            frameUrl: 'gonnect-camera:frame',
            frameFormats: [ 'I420', 'RGBX' ],
            headerSize: 12
        }

        const source = `(${runWorker.toString()})(${JSON.stringify(settings)})`
        return URL.createObjectURL(new Blob([ source ], { type: 'text/javascript' }))
    }

    // Runs in worker (no closures)
    function runWorker(settings) {
        self.onmessage = async event => {
            const writer = event.data.writable.getWriter()
            let width = 0
            let height = 0

            try {
                while (true) {
                    const response = await fetch(settings.frameUrl, { cache: 'no-store' })
                    const frame = toVideoFrame(await response.arrayBuffer())

                    if (frame.codedWidth !== width || frame.codedHeight !== height) {
                        width = frame.codedWidth
                        height = frame.codedHeight
                        self.postMessage({ type: 'size', width, height })
                    }

                    try {
                        await writer.write(frame)
                    } finally {
                        frame.close()
                    }
                }
            } catch (error) {
                self.postMessage({ type: 'failed', message: error.message })
            }
        }

        function toVideoFrame(buffer) {
            const header = new DataView(buffer, 0, settings.headerSize)

            return new VideoFrame(new Uint8Array(buffer, settings.headerSize), {
                format: settings.frameFormats[header.getUint32(0, true)],
                codedWidth: header.getUint32(4, true),
                codedHeight: header.getUint32(8, true),
                timestamp: Math.round(performance.now() * 1000)
            })
        }
    }
})()

