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
#include <QtNetwork/QNetworkAccessManager>

Q_DECLARE_LOGGING_CATEGORY(WebRTCSignalingLog)

class QNetworkReply;

/// Handles WebRTC signaling over HTTP
/// Exchanges SDP offers/answers and ICE candidates with remote peer
class WebRTCSignaling : public QObject
{
    Q_OBJECT

public:
    explicit WebRTCSignaling(const QString &peerUrl, QObject *parent = nullptr);
    ~WebRTCSignaling() override;

    /// Send SDP offer to peer
    void sendOffer(const QString &sdp);

    /// Send ICE candidate to peer
    void sendIceCandidate(const QString &candidate, int sdpMLineIndex);

    /// Change peer URL
    void setPeerUrl(const QString &url);

    /// Check if signaling server is reachable
    void checkHealth();

    QString peerUrl() const { return _peerUrl; }

signals:
    /// Emitted when SDP answer is received from peer
    void answerReceived(const QString &sdp);

    /// Emitted when ICE candidate is received from peer
    void iceCandidateReceived(const QString &candidate, int sdpMLineIndex);

    /// Emitted when health check completes
    void healthCheckResult(bool reachable);

    /// Emitted on error
    void errorOccurred(const QString &error);

private slots:
    void _onOfferReplyFinished();
    void _onIceReplyFinished();
    void _onHealthCheckFinished();

private:
    QNetworkAccessManager _networkManager;
    QString _peerUrl;  // e.g., "http://192.168.1.100:8080"
};
