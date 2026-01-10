/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QString>

#ifdef QGC_GST_STREAMING
// Define before including GStreamer headers to suppress #warning directives
#ifndef GST_USE_UNSTABLE_API
#define GST_USE_UNSTABLE_API
#endif

#include <gst/gst.h>
#include <gst/webrtc/webrtc.h>
#endif

Q_DECLARE_LOGGING_CATEGORY(GstVoiceStreamerLog)

class GstVoiceStreamer : public QObject
{
    Q_OBJECT

public:
    explicit GstVoiceStreamer(QObject *parent = nullptr);
    ~GstVoiceStreamer() override;

    /// Initialize GStreamer (must be called before use)
    bool initialize();

    /// Start the voice streaming pipeline
    /// @param stunServer STUN server URL (e.g., "stun://stun.l.google.com:19302")
    /// @param turnServer Optional TURN server URL
    bool startPipeline(const QString &stunServer = QString(), const QString &turnServer = QString());

    /// Stop the voice streaming pipeline
    void stopPipeline();

    /// Handle remote SDP offer
    void handleOfferSdp(const QString &sdp);

    /// Handle remote SDP answer
    void handleAnswerSdp(const QString &sdp);

    /// Handle remote ICE candidate
    void handleIceCandidate(const QString &candidate, int sdpMLineIndex);

    /// Set mute state (for push-to-talk)
    void setMuted(bool muted);

    /// Set volume level (0-100)
    void setVolume(int volume);

    bool isStreaming() const { return _streaming; }
    bool isInitialized() const { return _initialized; }

signals:
    /// Emitted when local SDP is generated (to be sent to peer)
    void localSdpGenerated(const QString &type, const QString &sdp);

    /// Emitted when local ICE candidate is generated
    void iceCandidateGenerated(const QString &candidate, int sdpMLineIndex);

    /// Emitted when streaming state changes
    void streamingStateChanged(bool active);

    /// Emitted when WebRTC connection state changes
    void connectionStateChanged(const QString &state);

    /// Emitted on error
    void errorOccurred(const QString &error);

private:
#ifdef QGC_GST_STREAMING
    void _createPipeline();
    void _destroyPipeline();
    void _createOffer();

    // GStreamer callbacks (must be static)
    static void _onNegotiationNeeded(GstElement *webrtc, gpointer userData);
    static void _onOfferCreated(GstPromise *promise, gpointer userData);
    static void _onIceCandidate(GstElement *webrtc, guint mlineindex, gchar *candidate, gpointer userData);
    static void _onConnectionStateChanged(GstElement *webrtc, GParamSpec *pspec, gpointer userData);
    static void _onIceConnectionStateChanged(GstElement *webrtc, GParamSpec *pspec, gpointer userData);
    static GstBusSyncReply _onBusMessage(GstBus *bus, GstMessage *msg, gpointer userData);
    static GstPadProbeReturn _audioPadProbe(GstPad *pad, GstPadProbeInfo *info, gpointer userData);
    static GstPadProbeReturn _rtpPadProbe(GstPad *pad, GstPadProbeInfo *info, gpointer userData);
    void _checkMediaFlow();
    void _onMediaFlowTimeout();

    GstElement *_pipeline = nullptr;
    GstElement *_webrtcbin = nullptr;
    GstElement *_audioSrc = nullptr;
    GstElement *_opusEnc = nullptr;
    GstElement *_volume = nullptr;

    QString _stunServer;
    QString _turnServer;

    bool _initialized = false;
    bool _streaming = false;
    bool _isOfferer = true;  // QGC initiates connection
    int _volumeLevel = 80;  // Default volume level (0-100)
    bool _isMuted = false;

    QTimer *_mediaFlowTimer = nullptr;
    qint64 _lastRtpPacketTime = 0;
    bool _mediaFlowDetected = false;
#else
    bool _initialized = false;
    bool _streaming = false;
#endif
};
