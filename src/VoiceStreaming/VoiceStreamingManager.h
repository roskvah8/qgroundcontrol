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
#include <QtCore/QTimer>
#include <QtQmlIntegration/QtQmlIntegration>

Q_DECLARE_LOGGING_CATEGORY(VoiceStreamingManagerLog)

class GstVoiceStreamer;
class WebRTCSignaling;
class VoiceStreamingSettings;

class VoiceStreamingManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

    Q_PROPERTY(bool streaming READ streaming NOTIFY streamingChanged)
    Q_PROPERTY(bool pushToTalkActive READ pushToTalkActive WRITE setPushToTalkActive NOTIFY pushToTalkActiveChanged)
    Q_PROPERTY(QString connectionStatus READ connectionStatus NOTIFY connectionStatusChanged)
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(QString peerAddress READ peerAddress WRITE setPeerAddress NOTIFY peerAddressChanged)

public:
    explicit VoiceStreamingManager(QObject *parent = nullptr);
    ~VoiceStreamingManager() override;

    static VoiceStreamingManager *instance();

    bool streaming() const { return _streaming; }
    bool pushToTalkActive() const { return _pushToTalkActive; }
    QString connectionStatus() const { return _connectionStatus; }
    bool connected() const { return _connected; }
    QString peerAddress() const { return _peerAddress; }

    void setPeerAddress(const QString &address);

    /// Start streaming to configured peer address
    Q_INVOKABLE void startStreaming();

    /// Stop streaming
    Q_INVOKABLE void stopStreaming();

    /// Set push-to-talk state (true = transmitting, false = muted)
    Q_INVOKABLE void setPushToTalkActive(bool active);

    /// Set voice volume level (0-100)
    Q_INVOKABLE void setVoiceVolume(int volume);

    /// Initialize the voice streaming manager (called after QML is ready)
    void init();

signals:
    void streamingChanged();
    void pushToTalkActiveChanged();
    void connectionStatusChanged();
    void connectedChanged();
    void peerAddressChanged();
    void errorOccurred(const QString &error);

private slots:
    void _onStreamingStateChanged(bool active);
    void _onConnectionStatusChanged(const QString &status);
    void _onLocalSdpGenerated(const QString &type, const QString &sdp);
    void _onIceCandidateGenerated(const QString &candidate, int sdpMLineIndex);
    void _onAnswerReceived(const QString &sdp);
    void _onRemoteIceCandidateReceived(const QString &candidate, int sdpMLineIndex);
    void _onSignalingError(const QString &error);
    void _onStreamerError(const QString &error);
    void _onConnectionTimeout();
    void _voiceStreamingEnabledChanged();
    void _voicePeerAddressChanged();
    void _onSignalingHealthCheck();
    void _onHealthCheckResult(bool reachable);

private:
    void _initializeGStreamer();
    void _updateConnectionStatus();
    void _startConnectionTimeout();
    void _stopConnectionTimeout();
    void _startHeartbeat();
    void _stopHeartbeat();
    void _restartConnection();

    GstVoiceStreamer *_streamer = nullptr;
    WebRTCSignaling *_signaling = nullptr;
    VoiceStreamingSettings *_voiceStreamingSettings = nullptr;

    QTimer *_connectionTimeoutTimer = nullptr;
    QTimer *_heartbeatTimer = nullptr;

    bool _streaming = false;
    bool _pushToTalkActive = false;
    bool _connected = false;
    QString _connectionStatus;
    QString _peerAddress;

    bool _gstreamerInitialized = false;
    int _heartbeatFailureCount = 0;

    static constexpr int CONNECTION_TIMEOUT_MS = 10000;
    static constexpr int HEARTBEAT_INTERVAL_MS = 5000;  // Check every 5 seconds
    static constexpr int HEARTBEAT_FAILURE_THRESHOLD = 2;  // Disconnect after 2 consecutive failures
};
