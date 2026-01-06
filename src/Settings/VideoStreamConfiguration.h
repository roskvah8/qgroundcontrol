/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QJsonObject>
#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtQmlIntegration/QtQmlIntegration>

Q_DECLARE_LOGGING_CATEGORY(VideoStreamConfigurationLog)

/// Represents a single video stream configuration
/// Stores stream name, type, URL, and enabled state
class VideoStreamConfiguration : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("VideoStreamConfiguration should not be created in QML")

    Q_PROPERTY(QString name     READ name      WRITE setName      NOTIFY nameChanged)
    Q_PROPERTY(QString type     READ type      WRITE setType      NOTIFY typeChanged)
    Q_PROPERTY(QString url      READ url       WRITE setUrl       NOTIFY urlChanged)
    Q_PROPERTY(bool    enabled  READ enabled   WRITE setEnabled   NOTIFY enabledChanged)

public:
    explicit VideoStreamConfiguration(QObject* parent = nullptr);
    VideoStreamConfiguration(const QString& name, const QString& type, const QString& url, bool enabled, QObject* parent = nullptr);
    ~VideoStreamConfiguration() override = default;

    // Stream type constants (matching VideoSettings)
    static constexpr const char* TYPE_RTSP      = "RTSP Video Stream";
    static constexpr const char* TYPE_UDP_H264  = "UDP h.264 Video Stream";
    static constexpr const char* TYPE_UDP_H265  = "UDP h.265 Video Stream";
    static constexpr const char* TYPE_TCP       = "TCP-MPEG2 Video Stream";
    static constexpr const char* TYPE_MPEGTS    = "MPEG-TS Video Stream";

    // Property accessors
    QString name() const { return _name; }
    QString type() const { return _type; }
    QString url() const { return _url; }
    bool enabled() const { return _enabled; }

    void setName(const QString& name);
    void setType(const QString& type);
    void setUrl(const QString& url);
    void setEnabled(bool enabled);

    // JSON serialization
    [[nodiscard]] QJsonObject toJson() const;
    static VideoStreamConfiguration* fromJson(const QJsonObject& json, QObject* parent = nullptr);

    // URL validation
    [[nodiscard]] bool isValid() const;
    [[nodiscard]] QString validationError() const;

signals:
    void nameChanged(QString name);
    void typeChanged(QString type);
    void urlChanged(QString url);
    void enabledChanged(bool enabled);

private:
    QString _name;
    QString _type;
    QString _url;
    bool _enabled = true;
};
