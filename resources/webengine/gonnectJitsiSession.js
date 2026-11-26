// Run conference with lib-jitsi-meet
// See src/conference/JitsiSessionProtocol.h

(() => {
    'use strict'
    
    // Also show errors in async functions
    window.addEventListener('unhandledrejection', event => {
        console.error('GOnnect session unhandled error:', event.reason?.stack ?? event.reason)
    })

    const settings = __GONNECT_SESSION_CONFIG__
    const roomName = settings.roomName.toLowerCase()
    const control = createControl()

    const cameraDeviceId = 'gonnect-camera'  // As in gonnectCamera.js

    let connection = null
    let conference = null
    let audioTrack = null
    let videoTrack = null
    let requestedReceiverConstraints = null
    let sentTracks = ''
    let isLeaving = false

    // Serializes camera changes (track creation is asynchronous)
    let videoChange = Promise.resolve()

    // Remote audio track -> element playing it
    const remoteAudioElements = new Map()

    control.on('leave', leave)
    control.on('setAudioMuted', message => setAudioMuted(message.isMuted))
    control.on('setVideoMuted', message => {
        videoChange = videoChange.then(() => setVideoMuted(message.isMuted))
    })
    control.on('setAudioInputDevice', message => setAudioInputDevice(message.deviceId))
    control.on('setAudioOutputDevice', message => setAudioOutputDevice(message.deviceId))
    control.on('kickParticipant', message => conference?.kickParticipant(message.participantId))
    control.on('grantModerator', message => conference?.grantOwner(message.participantId))
    control.on('muteAll', muteAll)
    control.on('setHandRaised', message => setHandRaised(message.isRaised))
    control.on('sendMessage', message => conference?.sendTextMessage(message.text))
    control.on('joinWithPassword', message => conference?.join(message.password))
    control.on('setRoomPassword', message => setRoomPassword(message.password))
    control.on('setLobbyEnabled', message => setLobbyEnabled(message.isEnabled))
    control.on('answerLobbyUser', message => answerLobbyUser(message.participantId, message.isApproved))
    control.on('receiverConstraints', message => {
        requestedReceiverConstraints = message
        applyReceiverConstraints()
    })

    if (typeof JitsiMeetJS === 'undefined' || typeof config === 'undefined') {
        fail('libraryNotLoaded')
        return
    }

    connect()

    function connect() {
        JitsiMeetJS.setLogLevel(JitsiMeetJS.logLevels.WARN)
        JitsiMeetJS.init({ ...config })

        connection = new JitsiMeetJS.JitsiConnection(null, 
                                                     settings.jwt || null,
                                                     { ...config, serviceUrl: serviceUrl() })

        const events = JitsiMeetJS.events.connection
        connection.addEventListener(events.CONNECTION_ESTABLISHED, () => {
            joinConference().catch(error => {
                joinConference().catch(error => {
                    console.error('GOnnect session join error:', error?.stack ?? error)
                    fail('conference.setupFailed')
                })
            })
        })
        connection.addEventListener(events.CONNECTION_FAILED, error => fail(`connection.${error}`))
        connection.addEventListener(events.CONNECTION_DISCONNECTED, () => {
            if (!isLeaving) {
                fail('connection.disconnected')
            }
        })

        connection.connect({ name: roomName })
    }

    function serviceUrl() {
        const url = config.websocket || config.bosh
        const scheme = config.websocket ? 'wss:' : 'https:'
        const absoluteUrl = url.startsWith('//') ? `${scheme}${url}` : url
        const separator = absoluteUrl.includes('?') ? '&' : '?'
        return `${absoluteUrl}${separator}room=${encodeURIComponent(roomName)}`
    }

    async function joinConference() {
        conference = connection.initJitsiConference(roomName, conferenceOptions())
        applyReceiverConstraints()

        const events = JitsiMeetJS.events.conference
        conference.on(events.CONFERENCE_JOINED, () => {
            control.send({
                type: 'joined',
                participantId: conference.myUserId(),
                role: conference.getRole()
            })
            reportAudioMuted()
        })

        conference.on(events.USER_ROLE_CHANGED, (participantId, role) => {
            control.send({
                type: 'roleChanged',
                participantId,
                role
            })
        })

        conference.on(events.USER_JOINED, (participantId, participant) => {
            if (participant.isHidden()) {
                // Transcriber, recorder, etc.
                return
            }

            control.send({
                type: 'userJoined',
                participantId,
                displayName: participant.getDisplayName() ?? '',
                role: participant.getRole()
            })
        })

        conference.on(events.USER_LEFT, participantId => {
            control.send({
                type: 'userLeft',
                participantId
            })
            reportTracks()
        })

        conference.on(events.DISPLAY_NAME_CHANGED, (participantId, displayName) => {
            control.send({
                type: 'displayNameChanged',
                participantId,
                displayName
            })
        })

        conference.on(events.PARTICIPANT_PROPERTY_CHANGED, (participant, name, oldValue, value) => {
            if (name === 'raisedHand') {
                reportHandRaised(participant.getId(), handRaisedTimestamp(value))
            }
        })

        // When moderator wants to lower hand of someone else, an endpoint message is sent to that
        // peer who then sends the hand raise update
        conference.on(events.ENDPOINT_MESSAGE_RECEIVED, (participant, data) => {
            if (data?.name === 'lower-hand-message'
                && participant.isModerator()
                && !config.raisedHands?.disableLowerHandByModerator) {

                setHandRaised(false)
            }
        })

        conference.on(events.DOMINANT_SPEAKER_CHANGED, participantId => {
            control.send({ type: 'dominantSpeaker', participantId })
        })

        conference.on(events.MESSAGE_RECEIVED, (participantId, text, stamp, displayName) => {
            // Own messages are also received but already shown by GOnnect
            if (participantId === conference.myUserId()) {
                return
            }

            control.send({
                type: 'messageReceived',
                participantId: participantId || displayName,
                displayName: conference.getParticipantById(participantId)?.getDisplayName() ?? displayName ?? '',
                text,
                timestamp: messageTimestamp(stamp),
                isFromHistory: Boolean(stamp)
            })
        })

        // TODO: support private messages
        conference.on(events.PRIVATE_MESSAGE_RECEIVED, participantId => {
            console.info('GOnnect session ignores private message from', participantId)
        })

        conference.on(events.KICKED, () => fail('conference.kicked'))

        // Can be reported right after joining a locked room
        conference.on(events.LOCK_STATE_CHANGED, isLocked => {
            control.send({ type: 'roomLockChanged', isLocked })
        })

        // Reported right after joining room with lobby
        conference.on(events.MEMBERS_ONLY_CHANGED, isEnabled => {
            control.send({ type: 'lobbyChanged', isEnabled })
        })

        // Only moderators see lobby
        conference.on(events.LOBBY_USER_JOINED, (participantId, displayName) => {
            control.send({ type: 'lobbyUserJoined', participantId, displayName: displayName ?? '' })
        })
        conference.on(events.LOBBY_USER_LEFT, participantId => {
            control.send({ type: 'lobbyUserLeft', participantId })
        })

        conference.on(events.CONFERENCE_FAILED, error => {
            const errors = JitsiMeetJS.errors.conference

            // Ask again on wrong password
            if (error === errors.PASSWORD_REQUIRED) {
                control.send({ type: 'passwordRequired' })
                return
            }

            // Once a moderator lets this client in, the library joins on its own
            if (error === errors.MEMBERS_ONLY_ERROR) {
                joinLobby()
                return
            }
            fail(error)
        })
        conference.on(events.TRACK_ADDED, track => {
            playRemoteAudio(track)
            watchRemoteVideo(track)
            reportParticipantMuted(track)
            reportTracks()
        })
        conference.on(events.TRACK_REMOVED, track => {
            stopRemoteAudio(track)
            reportParticipantMuted(track)
            reportTracks()
        })
        conference.on(events.TRACK_MUTE_CHANGED, track => {
            reportParticipantMuted(track)
            reportTracks()
        })
        conference.on(events.PARTICIPANT_SOURCE_UPDATED, reportTracks)

        conference.setDisplayName(settings.displayName)

        audioTrack = await createAudioTrack(undefined, settings.isAudioMuted)
        if (audioTrack) {
            await conference.addTrack(audioTrack)
        }

        conference.join()

        reportDevices()
        JitsiMeetJS.mediaDevices.addEventListener(
            JitsiMeetJS.events.mediaDevices.DEVICE_LIST_CHANGED, reportDevices)
    }

    function conferenceOptions() {
        const codecPreferenceOrder = settings.videoCodecs
        return {
            ...config,
            videoQuality: { ...config.videoQuality, codecPreferenceOrder },
            p2p: { ...config.p2p, codecPreferenceOrder }
        }
    }

    function applyReceiverConstraints() {
        if (!conference || !requestedReceiverConstraints) {
            return 
        }

        const { constraints, onStageSources } = requestedReceiverConstraints
        conference.setReceiverConstraints({
            lastN: config.channelLastN ?? -1,
            defaultConstraints: { maxHeight: 0 },
            constraints,
            onStageSources,
            selectedSources: []
        })
    }

    function watchRemoteVideo(track) {
        if (track.isLocal() || !track.isVideoTrack()) {
            return
        }

        const events = JitsiMeetJS.events.track
        track.addEventListener(events.TRACK_VIDEOTYPE_CHANGED, reportTracks)
        track.addEventListener(events.TRACK_OWNER_SET, reportTracks)
    }

    function reportTracks() {
        if (!conference) {
            return
        }

        const tracks = conference.getParticipants()
            .flatMap(participant => participant.getTracksByMediaType('video'))
            .filter(track => track.getSourceName())
            .map(track => ({
                trackId: track.getTrack().id,
                sourceName: track.getSourceName(),
                participantId: track.getParticipantId(),
                videoType: track.getVideoType(),
                isMuted: track.isMuted()
            }))

        const message = { type: 'tracks', tracks }
        const snapshot = JSON.stringify(message)
        if (snapshot !== sentTracks) {
            sentTracks = snapshot
            control.send(message)
        }
    }

    function muteAll() {
        conference?.getParticipants()
            .filter(participant => !participant.isHidden())
            .forEach(participant => conference.muteParticipant(participant.getId(), 'audio'))
    }

    // Count participant without track as muted
    function reportParticipantMuted(track) {
        if (track.isLocal()) {
            return
        }

        const participant = conference.getParticipantById(track.getParticipantId())
        if (!participant || participant.isHidden()) {
            return
        }

        const cameraTracks = participant.getTracksByMediaType('video')
            .filter(cameraTrack => cameraTrack.getVideoType() === 'camera')

        control.send({
            type: 'participantMuted',
            participantId: participant.getId(),
            isAudioMuted: participant.isAudioMuted(),
            isVideoMuted: cameraTracks.every(cameraTrack => cameraTrack.isMuted())
        })
    }

    // Timestamp is ms since epoch or 0 if hand lowered (true|false from older clients)
    function handRaisedTimestamp(value) {
        if (value === 'true') {
            return Date.now()
        }

        const timestamp = parseInt(value, 10)
        return Number.isNaN(timestamp) ? 0 : timestamp
    }

    function messageTimestamp(stamp) {
        const timestamp = stamp ? new Date(stamp).getTime() : NaN
        return Number.isNaN(timestamp) ? Date.now() : timestamp
    }

    function setRoomPassword(password) {
        if (!conference) {
            return
        }

        const change = password ? conference.lock(password) : conference.unlock()
        change.catch(error => {
            console.error('GOnnect session cannot set room password:', error)
        })
    }

    async function joinLobby() {
        try {
            await conference.joinLobby(settings.displayName, '')
            control.send({ type: 'lobbyWaiting' })
        } catch (error) {
            console.error('GOnnect session cannot join lobby:', error)
            fail('conference.lobbyFailed')
        }
    }

    function setLobbyEnabled(isEnabled) {
        if (!conference) {
            return
        }

        if (isEnabled) {
            conference.enableLobby().catch(error => {
                console.error('GOnnect session cannot enable lobby:', error)
            })
        } else {
            conference.disableLobby()
        }
    }

    function answerLobbyUser(participantId, isApproved) {
        if (!conference) {
            return
        }

        if (isApproved) {
            conference.lobbyApproveAccess(participantId)
        } else {
            conference.lobbyDenyAccess(participantId)
        }
    }

    // Local participant has no property event
    function setHandRaised(isRaised) {
        if (!conference) {
            return
        }

        const timestamp = isRaised ? Date.now() : 0
        conference.setLocalParticipantProperty('raisedHand', timestamp)
        reportHandRaised(conference.myUserId(), timestamp)
    }

    function reportHandRaised(participantId, timestamp) {
        control.send({ type: 'handRaised', participantId, timestamp })
    }

    async function createAudioTrack(deviceId, isMuted) {
        try {
            const [ track ] = await JitsiMeetJS.createLocalTracks({ devices: [ 'audio' ],
                                                                    micDeviceId: deviceId })
            if (isMuted) {
                await track.mute()
            }

            track.addEventListener(JitsiMeetJS.events.track.TRACK_MUTE_CHANGED, () => {
                if (track === audioTrack) {
                    reportAudioMuted()
                }
            })
            return track
        } catch (error) {
            console.error('GOnnect session microphone not available:', error?.message ?? error)
            return null
        }
    }

    function reportAudioMuted() {
        if (audioTrack) {
            control.send({
                type: 'audioMuted',
                isMuted: audioTrack.isMuted()
            })
        }
    }

    async function setAudioMuted(isMuted) {
        if (!audioTrack) {
            return
        }

        // GOnnect waits for confirmation (even if nothing changed)
        if (audioTrack.isMuted() === isMuted) {
            reportAudioMuted()
            return
        }

        try {
            if (isMuted) {
                await audioTrack.mute()
            } else {
                await audioTrack.unmute()
            }
        } catch (error) {
            console.error('GOnnect session cannot change microphone mute', error?.message ?? error)
            reportAudioMuted()
        }
    }

    async function setAudioInputDevice(deviceId) {
        if (!conference || audioTrack?.getDeviceId() === deviceId) {
            return
        }

        const isMuted = audioTrack?.isMuted() ?? settings.isAudioMuted
        const track = await createAudioTrack(deviceId, isMuted)
        if (!track) {
            return
        }

        const previousTrack = audioTrack
        audioTrack = track
        await conference.replaceTrack(previousTrack, track)
        await previousTrack?.dispose()
    }

    async function setVideoMuted(isMuted) {
        try {
            if (!videoTrack && !isMuted) {
                const track = await createVideoTrack()
                try {
                    await conference.addTrack(track)
                    videoTrack = track
                } catch (error) {
                    await track.dispose()
                    throw error
                }
            } else if (videoTrack && videoTrack.isMuted() !== isMuted) {
                await (isMuted ? videoTrack.mute() : videoTrack.unmute())
            }
        } catch (error) {
            console.error('GOnnect session cannot change camera:', error?.message ?? error)
        }

        reportVideoMuted()
    }

    async function createVideoTrack() {
        const [ track ] = await JitsiMeetJS.createLocalTracks({
            devices: [ 'video' ],
            cameraDeviceId,
            resolution: config.resolution ?? 720
        })

        track.addEventListener(JitsiMeetJS.events.track.TRACK_MUTE_CHANGED, () => {
            if (track === videoTrack) {
                reportVideoMuted()
            }
        })
        return track
    }

    function reportVideoMuted() {
        control.send({
            type: 'videoMuted',
            isMuted: !videoTrack || videoTrack.isMuted()
        })
    }

    function setAudioOutputDevice(deviceId) {
        JitsiMeetJS.mediaDevices.setAudioOutputDevice(deviceId).catch(error => {
            console.error('GOnnect session cannot set audio output', error?.message ?? error)
        })
    }

    function reportDevices() {
        JitsiMeetJS.mediaDevices.enumerateDevices(devices => {
            const toDevice = device => ({
                deviceId: device.deviceId,
                groupId: device.groupId,
                label: device.label
            })

            control.send({
                type: 'devices',
                audioInputs: devices.filter(device => device.kind === 'audioinput').map(toDevice),
                audioOutputs: devices.filter(device => device.kind === 'audiooutput').map(toDevice)
            })
        })
    }

    function playRemoteAudio(track) {
        if (track.isLocal() || track.getType() !== 'audio') {
            return
        }

        const element = document.createElement('audio')
        element.autoplay = true
        document.body.appendChild(element)
        track.attach(element)
        remoteAudioElements.set(track, element)
    }

    function stopRemoteAudio(track) {
        const element = remoteAudioElements.get(track)
        if (!element) {
            return
        }

        track.detach(element)
        element.remove()
        remoteAudioElements.delete(track)
    }

    async function leave() {
        isLeaving = true

        try {
            await conference?.leave()
        } catch (error) {
            console.warn('GOnnect session: leaving failed', error?.message ?? error)
        }
        await audioTrack?.dispose()
        await videoTrack?.dispose()
        connection?.disconnect()
    }

    function fail(reason) {
        console.error('GOnnect session failed:', reason)
        control.send({ type: 'failed', reason })
    }

    function createControl() {
        const query = new URLSearchParams({ token: settings.token })
        const socket = new WebSocket(`${settings.endpoint}/control?${query}`)
        const pendingMessages = []
        const handlers = {}

        socket.onopen = () => pendingMessages.splice(0).forEach(message => socket.send(message))
        socket.onmessage = event => {
            try {
                const message = JSON.parse(event.data)
                handlers[message.type]?.(message)
            } catch (error) {
                console.error('GOnnect session invalid control message:', error.message)
            }
        }
        socket.onclose = () => console.warn('GOnnect session control connection closed')

        return {
            send: message => {
                const text = JSON.stringify(message)
                if (socket.readyState === WebSocket.OPEN) {
                    socket.send(text)
                } else {
                    pendingMessages.push(text)
                }
            },
            on: (type, handler) => {
                handlers[type] = handler
            }
        }
    }
})()
