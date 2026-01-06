/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "VideoStreamConfiguration.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QRegularExpression>

QGC_LOGGING_CATEGORY(VideoStreamConfigurationLog, "qgc.settings.videostreamconfiguration")

VideoStreamConfiguration::VideoStreamConfiguration(QObject* parent)
    : QObject(parent)
{
}

VideoStreamConfiguration::VideoStreamConfiguration(const QString& name, const QString& type, const QString& url, bool enabled, QObject* parent)
    : QObject(parent)
    , _name(name)
    , _type(type)
    , _url(url)
    , _enabled(enabled)
{
}

void VideoStreamConfiguration::setName(const QString& name)
{
    if (_name != name) {
        _name = name;
        emit nameChanged(_name);
    }
}

void VideoStreamConfiguration::setType(const QString& type)
{
    if (_type != type) {
        _type = type;
        emit typeChanged(_type);
    }
}

void VideoStreamConfiguration::setUrl(const QString& url)
{
    if (_url != url) {
        _url = url;
        emit urlChanged(_url);
    }
}

void VideoStreamConfiguration::setEnabled(bool enabled)
{
    if (_enabled != enabled) {
        _enabled = enabled;
        emit enabledChanged(_enabled);
    }
}

QJsonObject VideoStreamConfiguration::toJson() const
{
    QJsonObject json;
    json["name"] = _name;
    json["type"] = _type;
    json["url"] = _url;
    json["enabled"] = _enabled;
    return json;
}

VideoStreamConfiguration* VideoStreamConfiguration::fromJson(const QJsonObject& json, QObject* parent)
{
    if (!json.contains("name") || !json.contains("type") || !json.contains("url")) {
        qCWarning(VideoStreamConfigurationLog) << "Invalid JSON: missing required fields";
        return nullptr;
    }

    const QString name = json["name"].toString();
    const QString type = json["type"].toString();
    const QString url = json["url"].toString();
    const bool enabled = json.value("enabled").toBool(true); // Default to true if missing

    auto* config = new VideoStreamConfiguration(name, type, url, enabled, parent);

    if (!config->isValid()) {
        qCWarning(VideoStreamConfigurationLog) << "Invalid configuration loaded from JSON:" << config->validationError();
        delete config;
        return nullptr;
    }

    return config;
}

bool VideoStreamConfiguration::isValid() const
{
    // Check name is not empty
    if (_name.trimmed().isEmpty()) {
        return false;
    }

    // Check type is one of the valid types
    if (_type != TYPE_RTSP &&
        _type != TYPE_UDP_H264 &&
        _type != TYPE_UDP_H265 &&
        _type != TYPE_TCP &&
        _type != TYPE_MPEGTS) {
        return false;
    }

    // Check URL is not empty
    if (_url.trimmed().isEmpty()) {
        return false;
    }

    // Basic URL format validation based on type
    if (_type == TYPE_RTSP) {
        // RTSP URL should start with rtsp://
        if (!_url.startsWith("rtsp://", Qt::CaseInsensitive)) {
            return false;
        }
    } else if (_type == TYPE_UDP_H264 || _type == TYPE_UDP_H265 || _type == TYPE_MPEGTS) {
        // UDP URLs should be in format: ip:port or 0.0.0.0:port
        static const QRegularExpression udpRegex(R"(^[\d\.]+:\d+$)");
        if (!udpRegex.match(_url).hasMatch()) {
            return false;
        }
    } else if (_type == TYPE_TCP) {
        // TCP URLs should be in format: ip:port
        static const QRegularExpression tcpRegex(R"(^[\d\.]+:\d+$)");
        if (!tcpRegex.match(_url).hasMatch()) {
            return false;
        }
    }

    return true;
}

QString VideoStreamConfiguration::validationError() const
{
    if (_name.trimmed().isEmpty()) {
        return tr("Stream name cannot be empty");
    }

    if (_type != TYPE_RTSP &&
        _type != TYPE_UDP_H264 &&
        _type != TYPE_UDP_H265 &&
        _type != TYPE_TCP &&
        _type != TYPE_MPEGTS) {
        return tr("Invalid stream type: %1").arg(_type);
    }

    if (_url.trimmed().isEmpty()) {
        return tr("Stream URL cannot be empty");
    }

    if (_type == TYPE_RTSP) {
        if (!_url.startsWith("rtsp://", Qt::CaseInsensitive)) {
            return tr("RTSP URL must start with rtsp://");
        }
    } else if (_type == TYPE_UDP_H264 || _type == TYPE_UDP_H265 || _type == TYPE_MPEGTS) {
        static const QRegularExpression udpRegex(R"(^[\d\.]+:\d+$)");
        if (!udpRegex.match(_url).hasMatch()) {
            return tr("UDP URL must be in format: ip:port (e.g. 0.0.0.0:5600)");
        }
    } else if (_type == TYPE_TCP) {
        static const QRegularExpression tcpRegex(R"(^[\d\.]+:\d+$)");
        if (!tcpRegex.match(_url).hasMatch()) {
            return tr("TCP URL must be in format: ip:port");
        }
    }

    return QString(); // Valid
}
