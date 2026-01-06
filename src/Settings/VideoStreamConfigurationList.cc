/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "VideoStreamConfigurationList.h"
#include "VideoStreamConfiguration.h"
#include "VideoSettings.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QSettings>

QGC_LOGGING_CATEGORY(VideoStreamConfigurationListLog, "qgc.settings.videostreamconfigurationlist")

static constexpr const char* kStreamListKey = "VideoStreams/streamList";
static constexpr const char* kCurrentStreamIndexKey = "VideoStreams/currentStreamIndex";

VideoStreamConfigurationList::VideoStreamConfigurationList(QObject* parent)
    : QmlObjectListModel(parent)
{
}

QStringList VideoStreamConfigurationList::streamNames() const
{
    QStringList names;
    for (int i = 0; i < count(); i++) {
        // Use const_cast to call non-const get() from const method
        auto* config = qobject_cast<VideoStreamConfiguration*>(const_cast<VideoStreamConfigurationList*>(this)->get(i));
        if (config) {
            names.append(config->name());
        }
    }
    return names;
}

void VideoStreamConfigurationList::setCurrentStreamIndex(int index)
{
    if (index >= -1 && index < count() && _currentStreamIndex != index) {
        _currentStreamIndex = index;

        // Save to settings
        QSettings settings;
        settings.setValue(kCurrentStreamIndexKey, _currentStreamIndex);

        emit currentStreamIndexChanged(_currentStreamIndex);
    }
}

void VideoStreamConfigurationList::addStream(const QString& name, const QString& type, const QString& url, bool enabled)
{
    auto* config = new VideoStreamConfiguration(name, type, url, enabled, this);

    if (!config->isValid()) {
        qCWarning(VideoStreamConfigurationListLog) << "Cannot add invalid stream configuration:" << config->validationError();
        delete config;
        return;
    }

    // Connect to name changes to update streamNames property
    connect(config, &VideoStreamConfiguration::nameChanged, this, &VideoStreamConfigurationList::_onStreamNameChanged);

    append(config);
    saveToSettings();
    emit streamNamesChanged();
}

void VideoStreamConfigurationList::removeStream(int index)
{
    if (index < 0 || index >= count()) {
        qCWarning(VideoStreamConfigurationListLog) << "Invalid stream index:" << index;
        return;
    }

    const bool wasCurrentStream = (index == _currentStreamIndex);
    const int countBeforeRemoval = count();

    removeAt(index);

    // Update current stream index based on what was removed
    if (count() == 0) {
        // No streams left, disable video
        setCurrentStreamIndex(-1);
    } else if (wasCurrentStream) {
        // We removed the current stream, select the next available one
        // If we removed the last stream, select the new last stream
        int newIndex = (index < count()) ? index : count() - 1;
        setCurrentStreamIndex(newIndex);
        qCDebug(VideoStreamConfigurationListLog) << "Removed current stream, switched to stream" << newIndex;
    } else if (index < _currentStreamIndex) {
        // We removed a stream before the current one, adjust the index
        setCurrentStreamIndex(_currentStreamIndex - 1);
    }

    saveToSettings();
    emit streamNamesChanged();
}

VideoStreamConfiguration* VideoStreamConfigurationList::getStream(int index)
{
    if (index < 0 || index >= count()) {
        return nullptr;
    }
    return qobject_cast<VideoStreamConfiguration*>(get(index));
}

void VideoStreamConfigurationList::moveStreamUp(int index)
{
    if (!canMoveUp(index)) {
        qCWarning(VideoStreamConfigurationListLog) << "Cannot move stream up:" << index;
        return;
    }

    move(index, index - 1);

    // Update current stream index if needed
    if (_currentStreamIndex == index) {
        setCurrentStreamIndex(index - 1);
    } else if (_currentStreamIndex == index - 1) {
        setCurrentStreamIndex(index);
    }

    saveToSettings();
    emit streamNamesChanged();

    qCDebug(VideoStreamConfigurationListLog) << "Moved stream up from index" << index << "to" << (index - 1);
}

void VideoStreamConfigurationList::moveStreamDown(int index)
{
    if (!canMoveDown(index)) {
        qCWarning(VideoStreamConfigurationListLog) << "Cannot move stream down:" << index;
        return;
    }

    move(index, index + 1);

    // Update current stream index if needed
    if (_currentStreamIndex == index) {
        setCurrentStreamIndex(index + 1);
    } else if (_currentStreamIndex == index + 1) {
        setCurrentStreamIndex(index);
    }

    saveToSettings();
    emit streamNamesChanged();

    qCDebug(VideoStreamConfigurationListLog) << "Moved stream down from index" << index << "to" << (index + 1);
}

bool VideoStreamConfigurationList::canMoveUp(int index) const
{
    return index > 0 && index < count();
}

bool VideoStreamConfigurationList::canMoveDown(int index) const
{
    return index >= 0 && index < count() - 1;
}

void VideoStreamConfigurationList::saveToSettings()
{
    QJsonArray jsonArray;

    for (int i = 0; i < count(); i++) {
        auto* config = qobject_cast<VideoStreamConfiguration*>(get(i));
        if (config) {
            jsonArray.append(config->toJson());
        }
    }

    QJsonDocument doc(jsonArray);
    const QString jsonString = doc.toJson(QJsonDocument::Compact);

    QSettings settings;
    settings.setValue(kStreamListKey, jsonString);

    qCDebug(VideoStreamConfigurationListLog) << "Saved" << count() << "stream configurations to settings";
}

void VideoStreamConfigurationList::loadFromSettings()
{
    QSettings settings;
    const QString jsonString = settings.value(kStreamListKey).toString();

    if (jsonString.isEmpty()) {
        qCDebug(VideoStreamConfigurationListLog) << "No saved stream configurations found";
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(jsonString.toUtf8());
    if (!doc.isArray()) {
        qCWarning(VideoStreamConfigurationListLog) << "Invalid stream configuration JSON (not an array)";
        return;
    }

    beginResetModel();
    clear();

    const QJsonArray jsonArray = doc.array();
    for (const QJsonValue& value : jsonArray) {
        if (!value.isObject()) {
            qCWarning(VideoStreamConfigurationListLog) << "Skipping invalid stream configuration (not an object)";
            continue;
        }

        VideoStreamConfiguration* config = VideoStreamConfiguration::fromJson(value.toObject(), this);
        if (config) {
            // Connect to name changes to update streamNames property
            connect(config, &VideoStreamConfiguration::nameChanged, this, &VideoStreamConfigurationList::_onStreamNameChanged);
            append(config);
        }
    }

    endResetModel();

    // Load current stream index
    _currentStreamIndex = settings.value(kCurrentStreamIndexKey, -1).toInt();

    // Validate current stream index
    if (_currentStreamIndex >= count()) {
        _currentStreamIndex = -1;
    }

    qCDebug(VideoStreamConfigurationListLog) << "Loaded" << count() << "stream configurations from settings";
    emit streamNamesChanged();
}

void VideoStreamConfigurationList::migrateFromLegacySettings(VideoSettings* settings)
{
    if (!settings) {
        qCWarning(VideoStreamConfigurationListLog) << "Cannot migrate: null VideoSettings";
        return;
    }

    // Check if we already have streams configured (skip migration)
    if (count() > 0) {
        qCDebug(VideoStreamConfigurationListLog) << "Stream configurations already exist, skipping migration";
        return;
    }

    // Get current video source type
    const QString videoSource = settings->videoSource()->rawValue().toString();

    // Only migrate if there's an actual video source configured (not disabled or empty)
    if (videoSource.isEmpty() ||
        videoSource == settings->disabledVideoSource() ||
        videoSource == VideoSettings::videoSourceNoVideo) {
        qCDebug(VideoStreamConfigurationListLog) << "No legacy video source configured, skipping migration";
        return;
    }

    QString url;
    QString streamType;

    // Determine URL and type based on video source
    if (videoSource == settings->rtspVideoSource()) {
        url = settings->rtspUrl()->rawValue().toString();
        streamType = VideoStreamConfiguration::TYPE_RTSP;
    } else if (videoSource == settings->udp264VideoSource()) {
        url = settings->udpUrl()->rawValue().toString();
        streamType = VideoStreamConfiguration::TYPE_UDP_H264;
    } else if (videoSource == settings->udp265VideoSource()) {
        url = settings->udpUrl()->rawValue().toString();
        streamType = VideoStreamConfiguration::TYPE_UDP_H265;
    } else if (videoSource == settings->tcpVideoSource()) {
        url = settings->tcpUrl()->rawValue().toString();
        streamType = VideoStreamConfiguration::TYPE_TCP;
    } else if (videoSource == settings->mpegtsVideoSource()) {
        url = settings->udpUrl()->rawValue().toString();
        streamType = VideoStreamConfiguration::TYPE_MPEGTS;
    } else {
        qCDebug(VideoStreamConfigurationListLog) << "Unknown video source type, skipping migration:" << videoSource;
        return;
    }

    // Only create migration stream if URL is not empty
    if (!url.isEmpty()) {
        qCDebug(VideoStreamConfigurationListLog) << "Migrating legacy video configuration:" << streamType << url;

        addStream(tr("Default Stream"), streamType, url, true);
        setCurrentStreamIndex(0);

        qCInfo(VideoStreamConfigurationListLog) << "Successfully migrated legacy video configuration to multi-stream";
    }
}

void VideoStreamConfigurationList::_onStreamNameChanged()
{
    emit streamNamesChanged();
}
