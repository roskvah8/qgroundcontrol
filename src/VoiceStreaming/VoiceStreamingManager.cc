/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "VoiceStreamingManager.h"
#include "GstVoiceStreamer.h"
#include "WebRTCSignaling.h"
#include "QGCLoggingCategory.h"
#include "SettingsManager.h"
#include "VoiceStreamingSettings.h"

#include <QtCore/QDebug>

QGC_LOGGING_CATEGORY(VoiceStreamingManagerLog, "qgc.voicestreaming.manager")

Q_APPLICATION_STATIC(VoiceStreamingManager, _voiceStreamingManagerInstance);

VoiceStreamingManager::VoiceStreamingManager(QObject *parent)
    : QObject(parent)
    , _voiceStreamingSettings(SettingsManager::instance()->voiceStreamingSettings())
{
    qCDebug(VoiceStreamingManagerLog) << "VoiceStreamingManager created";

    // Load peer address from settings
    _peerAddress = _voiceStreamingSettings->voicePeerAddress()->rawValue().toString();

    // Create connection timeout timer
    _connectionTimeoutTimer = new QTimer(this);
    _connectionTimeoutTimer->setSingleShot(true);
    connect(_connectionTimeoutTimer, &QTimer::timeout, this, &VoiceStreamingManager::_onConnectionTimeout);

    // Create heartbeat timer for signaling server monitoring
    _heartbeatTimer = new QTimer(this);
    _heartbeatTimer->setInterval(HEARTBEAT_INTERVAL_MS);
    connect(_heartbeatTimer, &QTimer::timeout, this, &VoiceStreamingManager::_onSignalingHealthCheck);
}

VoiceStreamingManager::~VoiceStreamingManager()
{
    qCDebug(VoiceStreamingManagerLog) << "VoiceStreamingManager destroyed";

    stopStreaming();

    if (_streamer) {
        delete _streamer;
        _streamer = nullptr;
    }

    if (_signaling) {
        delete _signaling;
        _signaling = nullptr;
    }
}

VoiceStreamingManager *VoiceStreamingManager::instance()
{
    return _voiceStreamingManagerInstance();
}

void VoiceStreamingManager::init()
{
    qCDebug(VoiceStreamingManagerLog) << "Initializing VoiceStreamingManager";

    _initializeGStreamer();

    // Connect to settings changes
    connect(_voiceStreamingSettings->voiceStreamingEnabled(), &Fact::rawValueChanged,
            this, &VoiceStreamingManager::_voiceStreamingEnabledChanged);
    connect(_voiceStreamingSettings->voicePeerAddress(), &Fact::rawValueChanged,
            this, &VoiceStreamingManager::_voicePeerAddressChanged);

    // Auto-start if enabled and configured
    if (_voiceStreamingSettings->voiceStreamingEnabled()->rawValue().toBool() &&
        !_peerAddress.isEmpty()) {
        qCDebug(VoiceStreamingManagerLog) << "Auto-starting voice streaming on launch";
        startStreaming();
    }
}

void VoiceStreamingManager::_initializeGStreamer()
{
    if (_gstreamerInitialized) {
        return;
    }

#ifdef QGC_GST_STREAMING
    // Create streamer instance
    _streamer = new GstVoiceStreamer(this);

    // Connect streamer signals
    connect(_streamer, &GstVoiceStreamer::streamingStateChanged,
            this, &VoiceStreamingManager::_onStreamingStateChanged);
    connect(_streamer, &GstVoiceStreamer::connectionStateChanged,
            this, &VoiceStreamingManager::_onConnectionStatusChanged);
    connect(_streamer, &GstVoiceStreamer::localSdpGenerated,
            this, &VoiceStreamingManager::_onLocalSdpGenerated);
    connect(_streamer, &GstVoiceStreamer::iceCandidateGenerated,
            this, &VoiceStreamingManager::_onIceCandidateGenerated);
    connect(_streamer, &GstVoiceStreamer::errorOccurred,
            this, &VoiceStreamingManager::_onStreamerError);

    // Initialize GStreamer
    if (_streamer->initialize()) {
        _gstreamerInitialized = true;
        qCDebug(VoiceStreamingManagerLog) << "GStreamer initialized successfully";
    } else {
        qCWarning(VoiceStreamingManagerLog) << "Failed to initialize GStreamer - voice streaming unavailable";
    }
#else
    qCWarning(VoiceStreamingManagerLog) << "GStreamer support not compiled in - voice streaming unavailable";
#endif
}

void VoiceStreamingManager::setPeerAddress(const QString &address)
{
    if (_peerAddress == address) {
        return;
    }

    _peerAddress = address;
    emit peerAddressChanged();

    qCDebug(VoiceStreamingManagerLog) << "Peer address set to:" << address;
}

void VoiceStreamingManager::startStreaming()
{
    if (_streaming) {
        qCDebug(VoiceStreamingManagerLog) << "Already streaming";
        return;
    }

    if (_peerAddress.isEmpty()) {
        qCDebug(VoiceStreamingManagerLog) << "No peer address configured - skipping connection";
        return;
    }

    if (!_gstreamerInitialized) {
        qCDebug(VoiceStreamingManagerLog) << "GStreamer not initialized - skipping connection";
        return;
    }

#ifdef QGC_GST_STREAMING
    qCDebug(VoiceStreamingManagerLog) << "Starting voice streaming to" << _peerAddress;

    // Create signaling connection if needed
    if (!_signaling) {
        _signaling = new WebRTCSignaling(_peerAddress, this);

        // Connect signaling signals
        connect(_signaling, &WebRTCSignaling::answerReceived,
                this, &VoiceStreamingManager::_onAnswerReceived);
        connect(_signaling, &WebRTCSignaling::iceCandidateReceived,
                this, &VoiceStreamingManager::_onRemoteIceCandidateReceived);
        connect(_signaling, &WebRTCSignaling::errorOccurred,
                this, &VoiceStreamingManager::_onSignalingError);
        connect(_signaling, &WebRTCSignaling::healthCheckResult,
                this, &VoiceStreamingManager::_onHealthCheckResult);
    } else {
        _signaling->setPeerUrl(_peerAddress);
    }

    // Start the GStreamer pipeline
    // Use Google's STUN server by default
    const QString stunServer = QStringLiteral("stun://stun.l.google.com:19302");

    if (_streamer->startPipeline(stunServer)) {
        _streaming = true;
        _connectionStatus = tr("Connecting...");
        emit streamingChanged();
        emit connectionStatusChanged();

        // Start muted (wait for push-to-talk)
        _streamer->setMuted(true);
        qCDebug(VoiceStreamingManagerLog) << "Voice pipeline started (muted)";

        // Start connection timeout
        _startConnectionTimeout();
    } else {
        qCWarning(VoiceStreamingManagerLog) << "Failed to start voice pipeline - will retry";
    }
#endif
}

void VoiceStreamingManager::stopStreaming()
{
    if (!_streaming) {
        return;
    }

    qCDebug(VoiceStreamingManagerLog) << "Stopping voice streaming";

    // Stop connection timeout timer
    _stopConnectionTimeout();

    // Stop heartbeat monitoring
    _stopHeartbeat();

#ifdef QGC_GST_STREAMING
    if (_streamer) {
        _streamer->stopPipeline();
    }
#endif

    _streaming = false;
    _connected = false;
    _pushToTalkActive = false;
    _connectionStatus = tr("Disconnected");

    emit streamingChanged();
    emit connectedChanged();
    emit pushToTalkActiveChanged();
    emit connectionStatusChanged();
}

void VoiceStreamingManager::setPushToTalkActive(bool active)
{
    if (_pushToTalkActive == active) {
        return;
    }

    _pushToTalkActive = active;
    emit pushToTalkActiveChanged();

    qCDebug(VoiceStreamingManagerLog) << "Push-to-talk" << (active ? "ACTIVE" : "inactive");

#ifdef QGC_GST_STREAMING
    if (_streamer && _streaming) {
        // Unmute when PTT is active, mute when released
        _streamer->setMuted(!active);
    }
#endif
}

void VoiceStreamingManager::setVoiceVolume(int volume)
{
    qCDebug(VoiceStreamingManagerLog) << "Setting voice volume to" << volume;

#ifdef QGC_GST_STREAMING
    if (_streamer) {
        _streamer->setVolume(volume);
    }
#else
    Q_UNUSED(volume)
#endif
}

void VoiceStreamingManager::_onStreamingStateChanged(bool active)
{
    qCDebug(VoiceStreamingManagerLog) << "Streaming state changed:" << active;

    if (_streaming != active) {
        _streaming = active;
        emit streamingChanged();
    }

    if (!active) {
        _connected = false;
        _pushToTalkActive = false;
        emit connectedChanged();
        emit pushToTalkActiveChanged();
    }
}

void VoiceStreamingManager::_onConnectionStatusChanged(const QString &status)
{
    qCDebug(VoiceStreamingManagerLog) << "WebRTC connection state:" << status;

    _connectionStatus = status;
    emit connectionStatusChanged();

    // Update connected state based on WebRTC state
    const bool wasConnected = _connected;

    if (status == "connected") {
        _connected = true;
        // Stop connection timeout on successful connection
        _stopConnectionTimeout();
        // Start heartbeat monitoring
        _startHeartbeat();
        qCDebug(VoiceStreamingManagerLog) << "Peer connection established";
    } else if (status == "failed" || status == "closed" || status == "disconnected") {
        _connected = false;
        // Stop heartbeat when disconnected
        _stopHeartbeat();
        qCWarning(VoiceStreamingManagerLog) << "Peer connection lost:" << status;
    }

    if (_connected != wasConnected) {
        emit connectedChanged();

        if (!_connected && _streaming) {
            // Connection lost or failed, restart immediately
            qCWarning(VoiceStreamingManagerLog) << "Connection lost/failed, restarting connection";
            _restartConnection();
        }
    }
}

void VoiceStreamingManager::_onLocalSdpGenerated(const QString &type, const QString &sdp)
{
    qCDebug(VoiceStreamingManagerLog) << "Local SDP generated, type:" << type;

    if (!_signaling) {
        qCWarning(VoiceStreamingManagerLog) << "No signaling connection";
        return;
    }

    if (type == "offer") {
        // Send offer to peer via signaling
        _signaling->sendOffer(sdp);
    }
}

void VoiceStreamingManager::_onIceCandidateGenerated(const QString &candidate, int sdpMLineIndex)
{
    qCDebug(VoiceStreamingManagerLog) << "ICE candidate generated:" << candidate;

    if (!_signaling) {
        qCWarning(VoiceStreamingManagerLog) << "No signaling connection";
        return;
    }

    // Send ICE candidate to peer
    _signaling->sendIceCandidate(candidate, sdpMLineIndex);
}

void VoiceStreamingManager::_onAnswerReceived(const QString &sdp)
{
    qCDebug(VoiceStreamingManagerLog) << "SDP answer received from peer";

#ifdef QGC_GST_STREAMING
    if (_streamer) {
        _streamer->handleAnswerSdp(sdp);
    }
#endif
}

void VoiceStreamingManager::_onRemoteIceCandidateReceived(const QString &candidate, int sdpMLineIndex)
{
    qCDebug(VoiceStreamingManagerLog) << "Remote ICE candidate received:" << candidate;

#ifdef QGC_GST_STREAMING
    if (_streamer) {
        _streamer->handleIceCandidate(candidate, sdpMLineIndex);
    }
#endif
}

void VoiceStreamingManager::_onSignalingError(const QString &error)
{
    qCWarning(VoiceStreamingManagerLog) << "Signaling error:" << error;
    // Don't emit error to UI - we'll silently retry via reconnection logic
    // The reconnection mechanism will handle retrying automatically
}

void VoiceStreamingManager::_onStreamerError(const QString &error)
{
    qCWarning(VoiceStreamingManagerLog) << "Streamer error:" << error;
    // Don't show errors to user - reconnection will handle retrying
    // Errors are logged for debugging purposes only
}

void VoiceStreamingManager::_startConnectionTimeout()
{
    if (_connectionTimeoutTimer) {
        qCDebug(VoiceStreamingManagerLog) << "Starting connection timeout (" << CONNECTION_TIMEOUT_MS << "ms)";
        _connectionTimeoutTimer->start(CONNECTION_TIMEOUT_MS);
    }
}

void VoiceStreamingManager::_stopConnectionTimeout()
{
    if (_connectionTimeoutTimer && _connectionTimeoutTimer->isActive()) {
        qCDebug(VoiceStreamingManagerLog) << "Stopping connection timeout";
        _connectionTimeoutTimer->stop();
    }
}

void VoiceStreamingManager::_onConnectionTimeout()
{
    qCWarning(VoiceStreamingManagerLog) << "Connection timeout - no connection established within" << CONNECTION_TIMEOUT_MS << "ms";

    if (_streaming && !_connected) {
        // Connection timeout, restart immediately
        _restartConnection();
    }
}

void VoiceStreamingManager::_restartConnection()
{
    if (_peerAddress.isEmpty()) {
        qCDebug(VoiceStreamingManagerLog) << "Cannot restart connection - no peer address configured";
        return;
    }

    qCDebug(VoiceStreamingManagerLog) << "Restarting connection to" << _peerAddress;

    // Stop current streaming attempt
    if (_streaming) {
        stopStreaming();
    }

    // Immediately restart
    startStreaming();
}

void VoiceStreamingManager::_voiceStreamingEnabledChanged()
{
    bool enabled = _voiceStreamingSettings->voiceStreamingEnabled()->rawValue().toBool();

    if (enabled) {
        // Auto-start if peer address is configured
        if (!_peerAddress.isEmpty() && !_streaming) {
            qCDebug(VoiceStreamingManagerLog) << "Voice streaming enabled - starting";
            startStreaming();
        }
    } else {
        // Stop streaming when disabled
        if (_streaming) {
            qCDebug(VoiceStreamingManagerLog) << "Voice streaming disabled - stopping";
            stopStreaming();
        }
    }
}

void VoiceStreamingManager::_voicePeerAddressChanged()
{
    QString newAddress = _voiceStreamingSettings->voicePeerAddress()->rawValue().toString();

    if (_peerAddress != newAddress) {
        _peerAddress = newAddress;
        emit peerAddressChanged();

        // Restart if currently streaming
        if (_streaming) {
            qCDebug(VoiceStreamingManagerLog) << "Peer address changed - restarting connection";
            _restartConnection();
        }
        // Auto-start if enabled and new address is valid
        else if (_voiceStreamingSettings->voiceStreamingEnabled()->rawValue().toBool() &&
                 !_peerAddress.isEmpty()) {
            qCDebug(VoiceStreamingManagerLog) << "Peer address configured - starting streaming";
            startStreaming();
        }
    }
}

void VoiceStreamingManager::_startHeartbeat()
{
    if (!_heartbeatTimer) {
        return;
    }

    _heartbeatFailureCount = 0;
    _heartbeatTimer->start();
    qCDebug(VoiceStreamingManagerLog) << "Heartbeat monitoring started (interval:" << HEARTBEAT_INTERVAL_MS << "ms)";
}

void VoiceStreamingManager::_stopHeartbeat()
{
    if (_heartbeatTimer && _heartbeatTimer->isActive()) {
        _heartbeatTimer->stop();
        _heartbeatFailureCount = 0;
        qCDebug(VoiceStreamingManagerLog) << "Heartbeat monitoring stopped";
    }
}

void VoiceStreamingManager::_onSignalingHealthCheck()
{
    if (!_signaling) {
        return;
    }

    // Perform health check
    _signaling->checkHealth();
}

void VoiceStreamingManager::_onHealthCheckResult(bool reachable)
{
    if (reachable) {
        // Reset failure count on success
        _heartbeatFailureCount = 0;
    } else {
        // Increment failure count
        _heartbeatFailureCount++;
        qCWarning(VoiceStreamingManagerLog) << "Signaling server unreachable (failures:"
                                            << _heartbeatFailureCount << "/" << HEARTBEAT_FAILURE_THRESHOLD << ")";

        // If we've exceeded threshold, trigger disconnection
        if (_heartbeatFailureCount >= HEARTBEAT_FAILURE_THRESHOLD && _connected && _streaming) {
            qCWarning(VoiceStreamingManagerLog) << "Signaling server lost - peer assumed disconnected";

            // Stop heartbeat to prevent repeated disconnect triggers
            _stopHeartbeat();

            // Trigger disconnect and reconnect
            _connected = false;
            emit connectedChanged();
            _restartConnection();
        }
    }
}
