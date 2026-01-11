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

    const bool wasEmpty = (count() == 0);
    const bool hadNoValidSelection = (_currentStreamIndex < 0 || _currentStreamIndex >= count());

    append(config);

    // AUTO-SELECT: Select this stream if:
    // 1. This is the first stream (wasEmpty), OR
    // 2. No valid stream is currently selected (hadNoValidSelection), AND
    // 3. The new stream is enabled and valid
    if ((wasEmpty || hadNoValidSelection) && enabled && config->isValid()) {
        const int newStreamIndex = count() - 1;
        setCurrentStreamIndex(newStreamIndex);
        qCDebug(VideoStreamConfigurationListLog) << "Auto-selected new stream:" << name << "at index" << newStreamIndex;
    }

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

    // ENHANCED VALIDATION: Ensure currentStreamIndex points to valid, enabled stream
    if (_currentStreamIndex >= 0 && _currentStreamIndex < count()) {
        VideoStreamConfiguration* selectedStream = getStream(_currentStreamIndex);
        if (!selectedStream || !selectedStream->enabled() || !selectedStream->isValid()) {
            qCWarning(VideoStreamConfigurationListLog) << "Saved stream index" << _currentStreamIndex
                                                        << "points to invalid/disabled stream, resetting";
            _currentStreamIndex = -1;
        }
    } else if (_currentStreamIndex >= count()) {
        qCWarning(VideoStreamConfigurationListLog) << "Saved stream index" << _currentStreamIndex
                                                    << "out of range (count:" << count() << "), resetting";
        _currentStreamIndex = -1;
    }

    // AUTO-SELECT FALLBACK: If no valid selection but streams exist, pick first valid enabled stream
    if (_currentStreamIndex < 0 && count() > 0) {
        for (int i = 0; i < count(); i++) {
            VideoStreamConfiguration* candidate = getStream(i);
            if (candidate && candidate->enabled() && candidate->isValid()) {
                _currentStreamIndex = i;
                qCDebug(VideoStreamConfigurationListLog) << "Auto-selected first valid stream at index" << i
                                                          << ":" << candidate->name();

                // Save the auto-selected index
                QSettings settingsWriter;
                settingsWriter.setValue(kCurrentStreamIndexKey, _currentStreamIndex);
                break;
            }
        }
    }

    qCDebug(VideoStreamConfigurationListLog) << "Loaded" << count() << "stream configurations, current index:" << _currentStreamIndex;
    emit streamNamesChanged();
}

void VideoStreamConfigurationList::_onStreamNameChanged()
{
    emit streamNamesChanged();
}
