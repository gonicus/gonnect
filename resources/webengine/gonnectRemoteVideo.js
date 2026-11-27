// Taps remote video of Jitsi iframe and sends encoded frames to GOnnect.
// Endpoint and token are inserted via JitsiConnector::remoteVideoScriptInternal()
// See src/conference/JitsiVideoProtocol.h

(() => {
    'use strict'

    const config = __GONNECT_REMOTE_VIDEO_CONFIG__

    const storePollInterval = 500

    // Last codecs send to worker per tapped receiver
    const announcedCodecs = new WeakMap()

    // Jitsi app runs meeting in iframe, GOnnect session page in top frame
    if (!config.isSessionPage && window === window.top) {
        return
    }

    if (!('RTCRtpScriptTransform' in window)) {
        console.error('GOnnect remote video: RTCRtpScriptTransform not available')
        return
    }

    const worker = createWorker()
    tapPeerConnections()

    if (!config.isSessionPage) {
        hideJitsiVideos()
        connectControlWhenStoreAvailable()
    }

    function connectionUrl(path, parameters = {}) {
        const query = new URLSearchParams({ token: config.token, ...parameters })
        return `${config.endpoint}/${path}?${query}`
    }

    // Prevent browser from painting videos
    function hideJitsiVideos() {
        document.addEventListener('DOMContentLoaded', () => {
            const style = document.createElement('style')
            style.textContent = 'video { visibility: hidden !important; }'
            document.head.appendChild(style)
        })
    }

    function createWorker() {
        const settings = {
            protocolVersion: 2,
            headerSize: 12,
            keyFrameFlag: 0x01,
            maxBufferedBytes: 1024 * 1024,
            maxTrackedFrameIds: 512
        }

        const source = `(${runWorker.toString()})(${JSON.stringify(settings)})`
        const url = URL.createObjectURL(new Blob([ source ], { type: 'text/javascript' }))

        const createdWorker = new Worker(url)
        createdWorker.onerror = event => {
          console.error('GOnnect remote video: worker failed', event.message)
        }
        return createdWorker
    }

    // Runs in worker - must not be closure
    function runWorker(settings) {
        const codecsByTrack = new Map()
        const tracks = new Map()

        self.onmessage = event => {
            const { trackId, codecs } = event.data
            codecsByTrack.set(trackId, codecs)

            const track = tracks.get(trackId)
            if (track) {
                sendCodecs(trackId, track)
            }
        }

        self.onrtctransform = event => {
            const transformer = event.transformer
            const { trackId, url } = transformer.options
            const socket = new WebSocket(url)
            const track = { socket, transformer, isWaitingForKeyFrame: false, hasReceivedFrame: false }
            tracks.set(trackId, track)

            const isForChromium = createBaseLayerFilter()
            let hasReportedError = false
            const report = error => {
                if (!hasReportedError) {
                    hasReportedError = true
                    console.error('GOnnect remote video: frame handling failed', trackId, error.message)
                }
            }

            socket.onopen = () => sendCodecs(trackId, track)
            socket.onmessage = message => {
                // Chromium rejects requests before first frame
                if (typeof message.data === 'string' && track.hasReceivedFrame
                    && JSON.parse(message.data).type === 'keyFrameRequest') {
                    requestKeyFrame(transformer)
                }
            }

            transformer.readable
                .pipeThrough(new TransformStream({
                    transform(frame, controller) {
                        track.hasReceivedFrame = true

                        // Exception ends pipe and video for Chromium
                        try {
                            send(track, frame)
                        } catch (error) {
                            report(error)
                        }

                        let isForwarded = true
                        try {
                            isForwarded = isForChromium(frame)
                        } catch (error) {
                            report(error)
                        }

                        if (isForwarded) {
                            controller.enqueue(frame)
                        }
                    }
                }))
                .pipeTo(transformer.writable)
                .catch(error => console.warn('GOnnect remote video: tap failed', trackId, error.message))
                .finally(() => {
                    socket.close()
                    if (tracks.get(trackId) === track) {
                        tracks.delete(trackId)
                        codecsByTrack.delete(trackId)
                    }
                })
        }

        // Chromium only gets temporal base layer (Jitsi's video is hidden)
        function createBaseLayerFilter() {
            const forwardedFrameIds = new Set()

            return frame => {
                const { frameId, temporalIndex, dependencies } = frame.getMetadata()

                // All frames needed w/o depencendy descriptor
                if (frameId === undefined || temporalIndex === undefined) {
                    return true
                }

                if (frame.type === 'key') {
                    forwardedFrameIds.clear()
                } else if (temporalIndex > 0) {
                    return false
                }

                const isDecodable = (dependencies ?? []).every(id => forwardedFrameIds.has(id))
                if (isDecodable) {
                    forwardedFrameIds.add(frameId)
                    if (forwardedFrameIds.size > settings.maxTrackedFrameIds) {
                        forwardedFrameIds.delete(forwardedFrameIds.values().next().value)
                    }
                }
                return isDecodable
            }
        }

        function send(track, frame) {
            const socket = track.socket
            if (socket.readyState !== WebSocket.OPEN) {
                return
            }

            const isKeyFrame = frame.type === 'key'
            if (socket.bufferedAmount > settings.maxBufferedBytes) {
                // Following frames would miss their references
                if (!track.isWaitingForKeyFrame || isKeyFrame) {
                    track.isWaitingForKeyFrame = true
                    requestKeyFrame(track.transformer)
                }
                return
            }

            if (track.isWaitingForKeyFrame && !isKeyFrame) {
                return
            }
            track.isWaitingForKeyFrame = false
            socket.send(encode(frame, isKeyFrame))
        }

        function encode(frame, isKeyFrame) {
            const metadata = frame.getMetadata()
            const payload = new Uint8Array(frame.data)
            const message = new Uint8Array(settings.headerSize + payload.byteLength)
            const header = new DataView(message.buffer)

            header.setUint8(0, settings.protocolVersion)
            header.setUint8(1, isKeyFrame ? settings.keyFrameFlag : 0)
            header.setUint8(2, metadata.payloadType ?? 0)
            header.setUint32(4, metadata.rtpTimestamp ?? frame.timestamp, true)
            header.setUint16(8, metadata.width ?? 0, true)
            header.setUint16(10, metadata.height ?? 0, true)
            message.set(payload, settings.headerSize)
            return message
        }

        function sendCodecs(trackId, track) {
            const codecs = codecsByTrack.get(trackId)
            if (codecs && track.socket.readyState === WebSocket.OPEN) {
                track.socket.send(JSON.stringify({ type: 'codecs', codecs }))
            }
        }

        function requestKeyFrame(transformer) {
            Promise.resolve()
                .then(() => transformer.sendKeyFrameRequest())
                .catch(error => console.warn('GOnnect remote video: key frame request failed', error.message))
        }
    }

    function tapPeerConnections() {
        const NativePeerConnection = window.RTCPeerConnection

        class TappedPeerConnection extends NativePeerConnection {
            constructor(...args) {
                super(...args)
                this.addEventListener('track', event => {
                  if (event.track.kind !== 'video') {
                    return
                  }

                  // Use try-catch or errors are only reported as "script error"
                  try {
                    tapTrack(event.receiver, event.track)
                  } catch (error) {
                    console.error('GOnnect remote video: cannot tap track', event.track.id, error.message)
                  }
                })
            }

            // Negotiated codecs are only known after a description has been applied
            async setLocalDescription(...args) {
                const result = await super.setLocalDescription(...args)
                announceCodecs(this)
                return result
            }

            async setRemoteDescription(...args) {
                const result = await super.setRemoteDescription(...args)
                announceCodecs(this)
                return result
            }
        }

        window.RTCPeerConnection = TappedPeerConnection
        if (window.webkitRTCPeerConnection) {
            window.webkitRTCPeerConnection = TappedPeerConnection
        }
    }

    function tapTrack(receiver, track) {
        receiver.transform = new RTCRtpScriptTransform(worker, {
            trackId: track.id,
            url: connectionUrl('track', { id: track.id }),
        })
    }

    function announceCodecs(peerConnection) {
        for (const receiver of peerConnection.getReceivers()) {
            if (!receiver.transform) {
                continue
            }

            const codecs = Object.fromEntries(receiver.getParameters().codecs
                .map(codec => [codec.payloadType, codec.mimeType]))
            const serialized = JSON.stringify(codecs)

            if (serialized !== announcedCodecs.get(receiver) && Object.keys(codecs).length > 0) {
                announcedCodecs.set(receiver, serialized)
                worker.postMessage({ trackId: receiver.track.id, codecs })
            }
        }
    }

    // GOnnect receiver constraints replace those of Jitsi
    function createReceiverConstraints(store) {
        let requested = null
        let conference = null
        let setJitsiConstraints = null

        const apply = () => {
            if (!requested || !setJitsiConstraints) {
                return
            }

            const state = store.getState()
            const lastN = state['features/base/lastn']?.lastN

            // Video quality
            const preferredHeight = state['features/video-quality']?.preferredVideoQuality
            const limit = height => (preferredHeight > 0 ? Math.min(height, preferredHeight) : height)
            const constraints = Object.fromEntries(Object.entries(requested.constraints)
                .map(([sourceName, { maxHeight }]) => [sourceName, { maxHeight: limit(maxHeight) }]))

            try {
                setJitsiConstraints({
                    ...(lastN !== undefined && { lastN }),
                    defaultConstraints: { maxHeight: 0 },
                    constraints,
                    onStageSources: requested.onStageSources,
                    selectedSources: []
                })
            } catch (error) {
                console.error('GOnnect remote video. cannot set receiver constraints', error.message)
            }
        }

        // Called by Jitsi on every change of layout, lastN oder video quality
        const takeOver = candidate => {
            if (!candidate || candidate === conference) {
                return
            }
            conference = candidate

            if (typeof candidate.setReceiverConstraints !== 'function') {
                console.error('GOnnect remote video: conference without receiver constraints')
                return
            }

            setJitsiConstraints = candidate.setReceiverConstraints.bind(candidate)
            candidate.setReceiverConstraints = jitsiContraints => {
                if (requested) {
                    apply()
                } else {
                    setJitsiConstraints(jitsiContraints)
                }
            }
            apply()
        }

        return {
            follow: () => takeOver(store.getState()['features/base/conference']?.conference),
            receive: message => {
                requested = message
                apply()
            }
        }
    }

    function connectControlWhenStoreAvailable() {
        const timer = setInterval(() => {
            const store = window.APP?.store
            if (store) {
                clearInterval(timer)
                checkJitsiState(store.getState())
                connectControl(store)
            }
        }, storePollInterval)
    }

    // Early warning if Jitsi changes internals this script relies on
    function checkJitsiState(state) {
        const requiredKeys = [
            'features/base/conference',
            'features/base/lastn',
            'features/base/tracks',
            'features/video-quality',
        ]

        const missingKeys = requiredKeys.filter(key => !(key in state))
        if (missingKeys.length > 0) {
            console.log('GOnnect remote video: Jitsi state lacks', missingKeys.join(', '))
        }
    }

    function connectControl(store) {
        const socket = new WebSocket(connectionUrl('control'))
        const receiverConstraints = createReceiverConstraints(store)
        let sentSnapshot = ''

        const sendSnapshot = () => {
            if (socket.readyState !== WebSocket.OPEN) {
                return
            }

            const snapshot = JSON.stringify({
                type: 'tracks',
                tracks: remoteVideoTracks(store.getState()),
            })
            if (snapshot !== sentSnapshot) {
                sentSnapshot = snapshot
                socket.send(snapshot)
            }
        }

        socket.onopen = sendSnapshot
        socket.onmessage = message => {
            try {
                const data = JSON.parse(message.data)
                if (data.type === 'receiverConstraints') {
                    receiverConstraints.receive(data)
                }
            } catch (error) {
                console.error('GOnnect remote video: invalid control message', error.message)
            }
        }
        socket.onclose = () => console.warn('GOnnect remote video: control connection closed')
        store.subscribe(sendSnapshot)
        store.subscribe(receiverConstraints.follow)
        receiverConstraints.follow()
    }

    function remoteVideoTracks(state) {
        return (state['features/base/tracks'] ?? [])
            .filter(track => !track.local && track.jitsiTrack?.isVideoTrack())
            .map(track => ({
                trackId: track.jitsiTrack.getTrack()?.id,
                sourceName: track.jitsiTrack.getSourceName(),
                participantId: track.participantId,
                videoType: track.videoType,
                isMuted: track.muted,
            }))
    }
})()
