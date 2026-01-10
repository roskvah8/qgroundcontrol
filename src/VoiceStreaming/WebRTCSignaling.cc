/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "WebRTCSignaling.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QDebug>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

QGC_LOGGING_CATEGORY(WebRTCSignalingLog, "qgc.voicestreaming.signaling")

WebRTCSignaling::WebRTCSignaling(const QString &peerUrl, QObject *parent)
    : QObject(parent)
    , _peerUrl(peerUrl)
{
    qCDebug(WebRTCSignalingLog) << "WebRTC signaling created for peer:" << peerUrl;
}

WebRTCSignaling::~WebRTCSignaling()
{
    qCDebug(WebRTCSignalingLog) << "WebRTC signaling destroyed";
}

void WebRTCSignaling::setPeerUrl(const QString &url)
{
    if (_peerUrl == url) {
        return;
    }

    _peerUrl = url;
    qCDebug(WebRTCSignalingLog) << "Peer URL changed to:" << url;
}

void WebRTCSignaling::sendOffer(const QString &sdp)
{
    if (_peerUrl.isEmpty()) {
        qCWarning(WebRTCSignalingLog) << "No peer URL configured";
        emit errorOccurred(tr("No peer URL configured"));
        return;
    }

    qCDebug(WebRTCSignalingLog) << "Sending SDP offer to peer";

    // Build JSON payload
    QJsonObject json;
    json["type"] = "offer";
    json["sdp"] = sdp;

    // Construct URL
    QString url = _peerUrl;
    if (!url.endsWith("/")) {
        url += "/";
    }
    url += "offer";

    // Create request with proper initialization to avoid MSVC parsing issues
    QUrl requestUrl(url);
    QNetworkRequest request(requestUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // Send POST request
    QByteArray jsonData = QJsonDocument(json).toJson(QJsonDocument::Compact);
    QNetworkReply *reply = _networkManager.post(request, jsonData);

    connect(reply, &QNetworkReply::finished,
            this, &WebRTCSignaling::_onOfferReplyFinished);

    qCDebug(WebRTCSignalingLog) << "SDP offer sent to:" << url;
}

void WebRTCSignaling::sendIceCandidate(const QString &candidate, int sdpMLineIndex)
{
    if (_peerUrl.isEmpty()) {
        qCWarning(WebRTCSignalingLog) << "No peer URL configured";
        return;
    }

    qCDebug(WebRTCSignalingLog) << "Sending ICE candidate to peer";

    // Build JSON payload
    QJsonObject json;
    json["candidate"] = candidate;
    json["sdpMLineIndex"] = sdpMLineIndex;

    // Construct URL
    QString url = _peerUrl;
    if (!url.endsWith("/")) {
        url += "/";
    }
    url += "ice-candidate";

    // Create request with proper initialization to avoid MSVC parsing issues
    QUrl requestUrl(url);
    QNetworkRequest request(requestUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // Send POST request
    QByteArray jsonData = QJsonDocument(json).toJson(QJsonDocument::Compact);
    QNetworkReply *reply = _networkManager.post(request, jsonData);

    connect(reply, &QNetworkReply::finished,
            this, &WebRTCSignaling::_onIceReplyFinished);

    qCDebug(WebRTCSignalingLog) << "ICE candidate sent to:" << url;
}

void WebRTCSignaling::_onOfferReplyFinished()
{
    auto *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        return;
    }

    // Ensure reply is deleted
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        qCWarning(WebRTCSignalingLog) << "Offer request failed:" << reply->errorString();
        emit errorOccurred(tr("Failed to send offer: %1").arg(reply->errorString()));
        return;
    }

    // Parse response
    QByteArray responseData = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(responseData);

    if (doc.isNull() || !doc.isObject()) {
        qCWarning(WebRTCSignalingLog) << "Invalid JSON response";
        emit errorOccurred(tr("Invalid response from peer"));
        return;
    }

    QJsonObject json = doc.object();

    // Check if we received an answer
    if (json.contains("type") && json["type"].toString() == "answer") {
        QString sdp = json["sdp"].toString();
        if (!sdp.isEmpty()) {
            qCDebug(WebRTCSignalingLog) << "Received SDP answer from peer";
            emit answerReceived(sdp);
        } else {
            qCWarning(WebRTCSignalingLog) << "Empty SDP in answer";
        }
    } else if (json.contains("status")) {
        // Just a status response, peer will send answer separately
        qCDebug(WebRTCSignalingLog) << "Peer received offer, status:" << json["status"].toString();
    } else {
        qCWarning(WebRTCSignalingLog) << "Unexpected response format";
    }
}

void WebRTCSignaling::_onIceReplyFinished()
{
    auto *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        return;
    }

    // Ensure reply is deleted
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        qCWarning(WebRTCSignalingLog) << "ICE candidate request failed:" << reply->errorString();
        // Don't emit error for ICE candidates - they're not critical
        return;
    }

    qCDebug(WebRTCSignalingLog) << "ICE candidate delivered successfully";
}

void WebRTCSignaling::checkHealth()
{
    if (_peerUrl.isEmpty()) {
        emit healthCheckResult(false);
        return;
    }

    // Simple HEAD request to check if server is reachable
    QUrl requestUrl(_peerUrl);
    QNetworkRequest request(requestUrl);
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);

    QNetworkReply *reply = _networkManager.head(request);

    connect(reply, &QNetworkReply::finished,
            this, &WebRTCSignaling::_onHealthCheckFinished);

    qCDebug(WebRTCSignalingLog) << "Health check sent to:" << _peerUrl;
}

void WebRTCSignaling::_onHealthCheckFinished()
{
    auto *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        return;
    }

    // Ensure reply is deleted
    reply->deleteLater();

    bool reachable = (reply->error() == QNetworkReply::NoError);

    if (!reachable) {
        qCDebug(WebRTCSignalingLog) << "Health check failed:" << reply->errorString();
    } else {
        qCDebug(WebRTCSignalingLog) << "Health check succeeded";
    }

    emit healthCheckResult(reachable);
}
