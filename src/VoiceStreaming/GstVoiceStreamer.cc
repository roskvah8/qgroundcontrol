/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "GstVoiceStreamer.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QDebug>
#include <QtCore/QMetaObject>
#include <QtCore/QTimer>
#include <QtCore/QDateTime>

#ifdef QGC_GST_STREAMING
// Define before including GStreamer headers to suppress #warning directives
#ifndef GST_USE_UNSTABLE_API
#define GST_USE_UNSTABLE_API
#endif

#include <gst/sdp/sdp.h>
#endif

QGC_LOGGING_CATEGORY(GstVoiceStreamerLog, "qgc.voicestreaming.gstreamer")

GstVoiceStreamer::GstVoiceStreamer(QObject *parent)
    : QObject(parent)
{
    qCDebug(GstVoiceStreamerLog) << "GstVoiceStreamer created";
}

GstVoiceStreamer::~GstVoiceStreamer()
{
    qCDebug(GstVoiceStreamerLog) << "GstVoiceStreamer destroyed";
    stopPipeline();
}

bool GstVoiceStreamer::initialize()
{
#ifdef QGC_GST_STREAMING
    if (_initialized) {
        return true;
    }

    GError *error = nullptr;

    // Initialize GStreamer (safe to call multiple times)
    if (!gst_init_check(nullptr, nullptr, &error)) {
        if (error) {
            qCWarning(GstVoiceStreamerLog) << "Failed to initialize GStreamer:" << error->message;
            g_error_free(error);
        }
        return false;
    }

    _initialized = true;
    qCDebug(GstVoiceStreamerLog) << "GStreamer initialized";
    return true;
#else
    qCWarning(GstVoiceStreamerLog) << "GStreamer support not compiled in";
    return false;
#endif
}

bool GstVoiceStreamer::startPipeline(const QString &stunServer, const QString &turnServer)
{
#ifdef QGC_GST_STREAMING
    if (_streaming) {
        qCDebug(GstVoiceStreamerLog) << "Already streaming";
        return false;
    }

    if (!_initialized) {
        qCDebug(GstVoiceStreamerLog) << "GStreamer not initialized";
        return false;
    }

    _stunServer = stunServer.isEmpty() ? QStringLiteral("stun://stun.l.google.com:19302") : stunServer;
    _turnServer = turnServer;

    _createPipeline();
    if (!_pipeline) {
        return false;
    }

    // Start pipeline
    GstStateChangeReturn ret = gst_element_set_state(_pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        qCWarning(GstVoiceStreamerLog) << "Failed to start pipeline";
        _destroyPipeline();
        emit errorOccurred(tr("Failed to start audio pipeline"));
        return false;
    }

    _streaming = true;
    emit streamingStateChanged(true);

    qCDebug(GstVoiceStreamerLog) << "Pipeline started";
    return true;
#else
    Q_UNUSED(stunServer)
    Q_UNUSED(turnServer)
    qCWarning(GstVoiceStreamerLog) << "GStreamer support not compiled in";
    return false;
#endif
}

void GstVoiceStreamer::stopPipeline()
{
#ifdef QGC_GST_STREAMING
    if (!_streaming) {
        return;
    }

    qCDebug(GstVoiceStreamerLog) << "Stopping pipeline";

    _destroyPipeline();

    _streaming = false;
    emit streamingStateChanged(false);

    qCDebug(GstVoiceStreamerLog) << "Pipeline stopped";
#endif
}

void GstVoiceStreamer::setMuted(bool muted)
{
#ifdef QGC_GST_STREAMING
    if (!_volume) {
        qCWarning(GstVoiceStreamerLog) << "Volume element not available";
        return;
    }

    _isMuted = muted;

    // Set volume to 0 when muted, otherwise use the configured volume level
    const gdouble volumeValue = muted ? 0.0 : (_volumeLevel / 100.0);
    g_object_set(_volume, "volume", volumeValue, nullptr);

    qCDebug(GstVoiceStreamerLog) << "Audio" << (muted ? "muted" : "unmuted") << "volume:" << volumeValue;
#else
    Q_UNUSED(muted)
#endif
}

void GstVoiceStreamer::setVolume(int volume)
{
#ifdef QGC_GST_STREAMING
    // Clamp volume to 0-100 range
    _volumeLevel = qBound(0, volume, 100);

    if (!_volume) {
        qCWarning(GstVoiceStreamerLog) << "Volume element not available";
        return;
    }

    // Only apply volume if not muted
    if (!_isMuted) {
        const gdouble volumeValue = _volumeLevel / 100.0;
        g_object_set(_volume, "volume", volumeValue, nullptr);
        qCDebug(GstVoiceStreamerLog) << "Voice volume set to" << _volumeLevel << "%" << "(" << volumeValue << ")";
    }
#else
    Q_UNUSED(volume)
#endif
}

void GstVoiceStreamer::handleOfferSdp(const QString &sdp)
{
#ifdef QGC_GST_STREAMING
    if (!_webrtcbin) {
        qCWarning(GstVoiceStreamerLog) << "No webrtcbin element";
        return;
    }

    qCDebug(GstVoiceStreamerLog) << "Handling remote offer SDP";

    GstSDPMessage *sdpMsg = nullptr;
    gst_sdp_message_new_from_text(sdp.toUtf8().constData(), &sdpMsg);

    GstWebRTCSessionDescription *offer = gst_webrtc_session_description_new(
        GST_WEBRTC_SDP_TYPE_OFFER, sdpMsg);

    GstPromise *promise = gst_promise_new();
    g_signal_emit_by_name(_webrtcbin, "set-remote-description", offer, promise);
    gst_promise_interrupt(promise);
    gst_promise_unref(promise);

    gst_webrtc_session_description_free(offer);

    qCDebug(GstVoiceStreamerLog) << "Remote offer SDP set";
#else
    Q_UNUSED(sdp)
#endif
}

void GstVoiceStreamer::handleAnswerSdp(const QString &sdp)
{
#ifdef QGC_GST_STREAMING
    if (!_webrtcbin) {
        qCWarning(GstVoiceStreamerLog) << "No webrtcbin element";
        return;
    }

    qCDebug(GstVoiceStreamerLog) << "Handling remote answer SDP";

    GstSDPMessage *sdpMsg = nullptr;
    gst_sdp_message_new_from_text(sdp.toUtf8().constData(), &sdpMsg);

    GstWebRTCSessionDescription *answer = gst_webrtc_session_description_new(
        GST_WEBRTC_SDP_TYPE_ANSWER, sdpMsg);

    GstPromise *promise = gst_promise_new();
    g_signal_emit_by_name(_webrtcbin, "set-remote-description", answer, promise);
    gst_promise_interrupt(promise);
    gst_promise_unref(promise);

    gst_webrtc_session_description_free(answer);

    qCDebug(GstVoiceStreamerLog) << "Remote answer SDP set";
#else
    Q_UNUSED(sdp)
#endif
}

void GstVoiceStreamer::handleIceCandidate(const QString &candidate, int sdpMLineIndex)
{
#ifdef QGC_GST_STREAMING
    if (!_webrtcbin) {
        qCWarning(GstVoiceStreamerLog) << "No webrtcbin element";
        return;
    }

    qCDebug(GstVoiceStreamerLog) << "Adding remote ICE candidate:" << candidate;

    g_signal_emit_by_name(_webrtcbin, "add-ice-candidate", sdpMLineIndex,
                          candidate.toUtf8().constData());
#else
    Q_UNUSED(candidate)
    Q_UNUSED(sdpMLineIndex)
#endif
}

#ifdef QGC_GST_STREAMING

void GstVoiceStreamer::_createPipeline()
{
    qCDebug(GstVoiceStreamerLog) << "Creating GStreamer pipeline";

    GError *error = nullptr;

    // Build pipeline description
    // autoaudiosrc: Automatically detect and use system audio source (microphone)
    // audioconvert: Convert audio format
    // audioresample: Resample to target rate
    // queue: Buffer with leak policy to drop old data if buffer is full
    // opusenc: Encode audio with Opus codec (optimized for voice)
    // rtpopuspay: Package Opus into RTP packets
    // webrtcbin: WebRTC implementation (handles ICE, DTLS, SRTP)
    const QString pipelineDesc = QString(
        "autoaudiosrc name=audiosrc "
        "! volume name=volume volume=0.0 "  // Start muted (for PTT)
        "! audioconvert "
        "! audioresample "
        "! audio/x-raw,rate=16000,channels=1 "
        "! queue max-size-time=100000000 leaky=downstream "
        "! opusenc name=encoder bitrate=24000 frame-size=20 dtx=true inband-fec=true "
        "! rtpopuspay "
        "! application/x-rtp,media=audio,encoding-name=OPUS,payload=96 "
        "! webrtcbin name=sendrecv stun-server=%1"
    ).arg(_stunServer);

    _pipeline = gst_parse_launch(pipelineDesc.toUtf8().constData(), &error);

    if (error) {
        qCWarning(GstVoiceStreamerLog) << "Failed to create pipeline:" << error->message;
        emit errorOccurred(tr("Pipeline creation failed: %1").arg(QString::fromUtf8(error->message)));
        g_error_free(error);
        return;
    }

    if (!_pipeline) {
        qCWarning(GstVoiceStreamerLog) << "Failed to create pipeline (null)";
        emit errorOccurred(tr("Failed to create audio pipeline"));
        return;
    }

    // Get elements
    _webrtcbin = gst_bin_get_by_name(GST_BIN(_pipeline), "sendrecv");
    _audioSrc = gst_bin_get_by_name(GST_BIN(_pipeline), "audiosrc");
    _opusEnc = gst_bin_get_by_name(GST_BIN(_pipeline), "encoder");
    _volume = gst_bin_get_by_name(GST_BIN(_pipeline), "volume");

    if (!_webrtcbin) {
        qCWarning(GstVoiceStreamerLog) << "Failed to get webrtcbin element";
        _destroyPipeline();
        return;
    }

    // Configure TURN server if provided
    if (!_turnServer.isEmpty()) {
        g_object_set(_webrtcbin, "turn-server", _turnServer.toUtf8().constData(), nullptr);
        qCDebug(GstVoiceStreamerLog) << "TURN server configured:" << _turnServer;
    }

    // Set bundle policy to maximize compatibility
    g_object_set(_webrtcbin, "bundle-policy", 3, nullptr); // GST_WEBRTC_BUNDLE_POLICY_MAX_BUNDLE

    // Connect WebRTC signals
    g_signal_connect(_webrtcbin, "on-negotiation-needed",
                     G_CALLBACK(_onNegotiationNeeded), this);
    g_signal_connect(_webrtcbin, "on-ice-candidate",
                     G_CALLBACK(_onIceCandidate), this);
    g_signal_connect(_webrtcbin, "notify::connection-state",
                     G_CALLBACK(_onConnectionStateChanged), this);
    g_signal_connect(_webrtcbin, "notify::ice-connection-state",
                     G_CALLBACK(_onIceConnectionStateChanged), this);

    qCDebug(GstVoiceStreamerLog) << "ICE connection state monitoring enabled";

    // Attach bus message handler for error/warning detection
    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(_pipeline));
    if (bus) {
        gst_bus_enable_sync_message_emission(bus);
        g_signal_connect(bus, "sync-message", G_CALLBACK(_onBusMessage), this);
        gst_object_unref(bus);
    }

    qCDebug(GstVoiceStreamerLog) << "Pipeline created successfully";
}

void GstVoiceStreamer::_destroyPipeline()
{
    if (_pipeline) {
        // Disconnect bus handler
        GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(_pipeline));
        if (bus) {
            g_signal_handlers_disconnect_by_data(bus, this);
            gst_object_unref(bus);
        }

        gst_element_set_state(_pipeline, GST_STATE_NULL);
        gst_object_unref(_pipeline);
        _pipeline = nullptr;
    }

    // Don't unref these - they're owned by the pipeline
    _webrtcbin = nullptr;
    _audioSrc = nullptr;
    _opusEnc = nullptr;
    _volume = nullptr;

    qCDebug(GstVoiceStreamerLog) << "Pipeline destroyed";
}

void GstVoiceStreamer::_createOffer()
{
    qCDebug(GstVoiceStreamerLog) << "Creating SDP offer";

    GstPromise *promise = gst_promise_new_with_change_func(
        _onOfferCreated, this, nullptr);

    g_signal_emit_by_name(_webrtcbin, "create-offer", nullptr, promise);
}

void GstVoiceStreamer::_onNegotiationNeeded(GstElement *webrtc, gpointer userData)
{
    Q_UNUSED(webrtc)
    auto *streamer = static_cast<GstVoiceStreamer*>(userData);

    qCDebug(GstVoiceStreamerLog) << "Negotiation needed, creating offer";

    // Marshal to Qt thread
    QMetaObject::invokeMethod(streamer, [streamer]() {
        streamer->_createOffer();
    }, Qt::QueuedConnection);
}

void GstVoiceStreamer::_onOfferCreated(GstPromise *promise, gpointer userData)
{
    auto *streamer = static_cast<GstVoiceStreamer*>(userData);

    const GstStructure *reply = gst_promise_get_reply(promise);
    GstWebRTCSessionDescription *offer = nullptr;
    gst_structure_get(reply, "offer", GST_TYPE_WEBRTC_SESSION_DESCRIPTION, &offer, nullptr);

    if (!offer) {
        qCWarning(GstVoiceStreamerLog) << "Failed to create offer";
        gst_promise_unref(promise);
        return;
    }

    // Set local description
    GstPromise *localPromise = gst_promise_new();
    g_signal_emit_by_name(streamer->_webrtcbin, "set-local-description", offer, localPromise);
    gst_promise_interrupt(localPromise);
    gst_promise_unref(localPromise);

    // Get SDP text
    gchar *sdpText = gst_sdp_message_as_text(offer->sdp);
    QString sdpStr = QString::fromUtf8(sdpText);
    g_free(sdpText);

    gst_webrtc_session_description_free(offer);
    gst_promise_unref(promise);

    // Marshal to Qt thread and emit signal
    QMetaObject::invokeMethod(streamer, [streamer, sdpStr]() {
        emit streamer->localSdpGenerated("offer", sdpStr);
    }, Qt::QueuedConnection);

    qCDebug(GstVoiceStreamerLog) << "SDP offer created and set as local description";
}

void GstVoiceStreamer::_onIceCandidate(GstElement *webrtc, guint mlineindex,
                                       gchar *candidate, gpointer userData)
{
    Q_UNUSED(webrtc)
    auto *streamer = static_cast<GstVoiceStreamer*>(userData);

    QString candidateStr = QString::fromUtf8(candidate);
    int mlineInt = static_cast<int>(mlineindex);

    // Marshal to Qt thread
    QMetaObject::invokeMethod(streamer, [streamer, candidateStr, mlineInt]() {
        emit streamer->iceCandidateGenerated(candidateStr, mlineInt);
    }, Qt::QueuedConnection);
}

void GstVoiceStreamer::_onConnectionStateChanged(GstElement *webrtc, GParamSpec *pspec,
                                                 gpointer userData)
{
    Q_UNUSED(pspec)
    auto *streamer = static_cast<GstVoiceStreamer*>(userData);

    GstWebRTCPeerConnectionState state;
    g_object_get(webrtc, "connection-state", &state, nullptr);

    QString stateStr;
    switch (state) {
        case GST_WEBRTC_PEER_CONNECTION_STATE_NEW:
            stateStr = "new";
            break;
        case GST_WEBRTC_PEER_CONNECTION_STATE_CONNECTING:
            stateStr = "connecting";
            break;
        case GST_WEBRTC_PEER_CONNECTION_STATE_CONNECTED:
            stateStr = "connected";
            break;
        case GST_WEBRTC_PEER_CONNECTION_STATE_DISCONNECTED:
            stateStr = "disconnected";
            break;
        case GST_WEBRTC_PEER_CONNECTION_STATE_FAILED:
            stateStr = "failed";
            break;
        case GST_WEBRTC_PEER_CONNECTION_STATE_CLOSED:
            stateStr = "closed";
            break;
        default:
            stateStr = "unknown";
    }

    // Marshal to Qt thread
    QMetaObject::invokeMethod(streamer, [streamer, stateStr]() {
        emit streamer->connectionStateChanged(stateStr);
    }, Qt::QueuedConnection);

    qCDebug(GstVoiceStreamerLog) << "WebRTC connection state:" << stateStr;
}

void GstVoiceStreamer::_onIceConnectionStateChanged(GstElement *webrtc, GParamSpec *pspec,
                                                     gpointer userData)
{
    Q_UNUSED(pspec)
    auto *streamer = static_cast<GstVoiceStreamer*>(userData);

    GstWebRTCICEConnectionState state;
    g_object_get(webrtc, "ice-connection-state", &state, nullptr);

    QString stateStr;
    switch (state) {
        case GST_WEBRTC_ICE_CONNECTION_STATE_NEW:
            stateStr = "new";
            break;
        case GST_WEBRTC_ICE_CONNECTION_STATE_CHECKING:
            stateStr = "checking";
            break;
        case GST_WEBRTC_ICE_CONNECTION_STATE_CONNECTED:
            stateStr = "connected";
            break;
        case GST_WEBRTC_ICE_CONNECTION_STATE_COMPLETED:
            stateStr = "completed";
            break;
        case GST_WEBRTC_ICE_CONNECTION_STATE_FAILED:
            stateStr = "failed";
            break;
        case GST_WEBRTC_ICE_CONNECTION_STATE_DISCONNECTED:
            stateStr = "disconnected";
            break;
        case GST_WEBRTC_ICE_CONNECTION_STATE_CLOSED:
            stateStr = "closed";
            break;
        default:
            stateStr = "unknown";
    }

    // Treat ICE failures as connection state changes
    // ICE state is more sensitive and detects peer loss faster
    if (state == GST_WEBRTC_ICE_CONNECTION_STATE_FAILED ||
        state == GST_WEBRTC_ICE_CONNECTION_STATE_DISCONNECTED ||
        state == GST_WEBRTC_ICE_CONNECTION_STATE_CLOSED) {

        qCWarning(GstVoiceStreamerLog) << "ICE connection state changed to:" << stateStr << "- peer disconnected";

        // Marshal to Qt thread and emit connection state change
        QMetaObject::invokeMethod(streamer, [streamer]() {
            emit streamer->connectionStateChanged("disconnected");
        }, Qt::QueuedConnection);
    }
    // Report when ICE successfully connects
    else if (state == GST_WEBRTC_ICE_CONNECTION_STATE_CONNECTED ||
             state == GST_WEBRTC_ICE_CONNECTION_STATE_COMPLETED) {

        qCDebug(GstVoiceStreamerLog) << "ICE connection state changed to:" << stateStr << "- peer connected";

        QMetaObject::invokeMethod(streamer, [streamer]() {
            emit streamer->connectionStateChanged("connected");
        }, Qt::QueuedConnection);
    } else {
        qCDebug(GstVoiceStreamerLog) << "ICE connection state:" << stateStr;
    }
}

GstBusSyncReply GstVoiceStreamer::_onBusMessage(GstBus *bus, GstMessage *msg, gpointer userData)
{
    Q_UNUSED(bus)
    auto *streamer = static_cast<GstVoiceStreamer*>(userData);

    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_ERROR: {
            GError *err = nullptr;
            gchar *debug = nullptr;
            gst_message_parse_error(msg, &err, &debug);

            QString errorMsg = QString("GStreamer error: %1 (%2)")
                .arg(err->message)
                .arg(debug ? debug : "no debug info");

            qCWarning(GstVoiceStreamerLog) << errorMsg;

            // Emit error and treat as connection failure
            QMetaObject::invokeMethod(streamer, [streamer, errorMsg]() {
                emit streamer->errorOccurred(errorMsg);
                emit streamer->connectionStateChanged("failed");
            }, Qt::QueuedConnection);

            g_error_free(err);
            g_free(debug);
            break;
        }

        case GST_MESSAGE_WARNING: {
            GError *err = nullptr;
            gchar *debug = nullptr;
            gst_message_parse_warning(msg, &err, &debug);

            qCWarning(GstVoiceStreamerLog) << "GStreamer warning:" << err->message
                                           << (debug ? debug : "");

            // DTLS warnings might indicate connectivity issues
            QString message(err->message);
            if (message.contains("dtls", Qt::CaseInsensitive) ||
                message.contains("srtp", Qt::CaseInsensitive)) {

                QMetaObject::invokeMethod(streamer, [streamer]() {
                    emit streamer->connectionStateChanged("disconnected");
                }, Qt::QueuedConnection);
            }

            g_error_free(err);
            g_free(debug);
            break;
        }

        case GST_MESSAGE_INFO: {
            GError *err = nullptr;
            gchar *debug = nullptr;
            gst_message_parse_info(msg, &err, &debug);

            qCDebug(GstVoiceStreamerLog) << "GStreamer info:" << err->message;

            g_error_free(err);
            g_free(debug);
            break;
        }

        case GST_MESSAGE_EOS: {
            qCWarning(GstVoiceStreamerLog) << "End of stream detected";

            // EOS might indicate peer disconnection
            QMetaObject::invokeMethod(streamer, [streamer]() {
                emit streamer->connectionStateChanged("disconnected");
            }, Qt::QueuedConnection);
            break;
        }

        default:
            break;
    }

    return GST_BUS_PASS;
}

#endif // QGC_GST_STREAMING
