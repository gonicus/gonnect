// Taps remote video of Jitsi iframe and sends encoded frames to GOnnect.
// Endpoint and token are inserted via JitsiConnector::remoteVideoScriptInternal()
// See src/conference/JitsiVideoProtocol.h

(() => {
    'use strict'

    const config = __GONNECT_REMOTE_VIDEO_CONFIG__

    const storePollInterval = 500
    const minReceiverHeight = 720

    // Last codecs send to worker per tapped receiver
    const announcedCodecs = new WeakMap()

    // Meeting must run in iframe
    if (window === window.top) {
        return
    }

    if (!('RTCRtpScriptTransform' in window)) {
        console.error('GOnnect remote video: RTCRtpScriptTransform not available')
        return
    }

    const worker = createWorker()
    tapPeerConnections()
    raiseReceiverConstraints()
    connectControlWhenStoreAvailable()

    function connectionUrl(path, parameters = {}) {
        const query = new URLSearchParams({ token: config.token, ...parameters })
        return `${config.endpoint}/${path}?${query}`
    }

    function createWorker() {
        const settings = {
            protocolVersion: 2,
            headerSize: 12,
            keyFrameFlag: 0x01,
            maxBufferedBytes: 1024 * 1024
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
            const track = { socket, transformer, isWaitingForKeyFrame: false }
            tracks.set(trackId, track)

            socket.onopen = () => sendCodecs(trackId, track)
            socket.onmessage = message => {
                if (typeof message.data === 'string'
                    && JSON.parse(message.data).type === 'keyFrameRequest') {
                    requestKeyFrame(transformer)
                }
            }

            transformer.readable
                .pipeThrough(new TransformStream({
                    transform(frame, controller) {
                        send(track, frame)
                        controller.enqueue(frame)
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
                    console.error('GOnnect remote video: cannot tap track', event.track.id, error.mesage)
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

    // Jitsi requests video in size of own tiles; zero height -> unsubscribe
    function raiseReceiverConstraints() {
        const raise = constraint => {
            if (!(constraint?.maxHeight > 0)) {
                return constraint
            }
            return { ...constraint, maxHeight: Math.max(constraint.maxHeight, minReceiverHeight) }
        }

        const rewrite = data => {
            if (typeof data !== 'string' || !data.includes('ReceiverVideoConstraints')) {
                return data
            }

            let message
            try {
                message = JSON.parse(data)
            } catch {
                return data
            }
            if (message.colibriClass !== 'ReceiverVideoConstraints') {
                return data
            }

            message.defaultConstraints = raise(message.defaultConstraints)
            for (const [ sourceName, constraint ] of Object.entries(message.constraints ?? {})) {
                message.constraints[sourceName] = raise(constraint)
            }
            return JSON.stringify(message)
        }

        // Keep Jitsi working if rewriting fails
        const rewriteSafely = data => {
            try {
                return rewrite(data)
            } catch (error) {
                console.error('GOnnect remote video: cannot rewrite receiver constraints', error.message)
                return data
            }
        }

        const webSocketSend = WebSocket.prototype.send
        WebSocket.prototype.send = function (data) {
            return webSocketSend.call(this, rewriteSafely(data))
        }

        const dataChannelSend = RTCDataChannel.prototype.send
        RTCDataChannel.prototype.send = function (data) {
            return dataChannelSend.call(this, rewriteSafely(data))
        }
    }

    function connectControlWhenStoreAvailable() {
        const timer = setInterval(() => {
            const store = window.APP?.store
            if (store) {
                clearInterval(timer)
                connectControl(store)
            }
        }, storePollInterval)
    }

    function connectControl(store) {
        const socket = new WebSocket(connectionUrl('control'))
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
        socket.onclose = () => console.warn('GOnnect remote video: control connection closed')
        store.subscribe(sendSnapshot)
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
