/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "QmlControls/QmlObjectListModel.h"

#include <QtCore/QLoggingCategory>
#include <QtCore/QString>
#include <QtCore/QStringList>

Q_DECLARE_LOGGING_CATEGORY(VideoStreamConfigurationListLog)

class VideoSettings;
class VideoStreamConfiguration;

/// Manages a list of video stream configurations
/// Provides persistence to QSettings and migration from legacy single-stream settings
class VideoStreamConfigurationList : public QmlObjectListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("VideoStreamConfigurationList should not be created in QML")

    Q_PROPERTY(int currentStreamIndex READ currentStreamIndex WRITE setCurrentStreamIndex NOTIFY currentStreamIndexChanged)
    Q_PROPERTY(QStringList streamNames READ streamNames NOTIFY streamNamesChanged)

public:
    explicit VideoStreamConfigurationList(QObject* parent = nullptr);
    ~VideoStreamConfigurationList() override = default;

    // Property accessors
    int currentStreamIndex() const { return _currentStreamIndex; }
    QStringList streamNames() const;

    void setCurrentStreamIndex(int index);

    // Stream management
    Q_INVOKABLE void addStream(const QString& name, const QString& type, const QString& url, bool enabled = true);
    Q_INVOKABLE void removeStream(int index);
    Q_INVOKABLE VideoStreamConfiguration* getStream(int index);
    Q_INVOKABLE void moveStreamUp(int index);
    Q_INVOKABLE void moveStreamDown(int index);
    Q_INVOKABLE bool canMoveUp(int index) const;
    Q_INVOKABLE bool canMoveDown(int index) const;

    // Persistence
    Q_INVOKABLE void saveToSettings();
    Q_INVOKABLE void loadFromSettings();
    Q_INVOKABLE void migrateFromLegacySettings(VideoSettings* settings);

signals:
    void currentStreamIndexChanged(int index);
    void streamNamesChanged();

private slots:
    void _onStreamNameChanged();

private:
    int _currentStreamIndex = -1;
};
