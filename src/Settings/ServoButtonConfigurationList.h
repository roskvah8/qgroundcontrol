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

Q_DECLARE_LOGGING_CATEGORY(ServoButtonConfigurationListLog)

class ServoButtonConfiguration;

/// Manages a list of servo button configurations
/// Provides persistence to QSettings and duplicate channel validation
class ServoButtonConfigurationList : public QmlObjectListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("ServoButtonConfigurationList should not be created in QML")

public:
    explicit ServoButtonConfigurationList(QObject *parent = nullptr);
    ~ServoButtonConfigurationList() override = default;

    // Button management
    Q_INVOKABLE bool addButton(const QString &name, int servoChannel, int defaultPwm, int activePwm,
                               int holdTime, int activeTime, bool enabledWhenArmed);
    Q_INVOKABLE void removeButton(int index);
    Q_INVOKABLE ServoButtonConfiguration *getButton(int index);
    Q_INVOKABLE bool updateButton(int index, const QString &name, int servoChannel, int defaultPwm,
                                  int activePwm, int holdTime, int activeTime, bool enabledWhenArmed);

    /// Returns true if any other button (excluding excludeIndex) uses this channel
    Q_INVOKABLE bool isChannelDuplicate(int servoChannel, int excludeIndex = -1) const;

    // Persistence
    Q_INVOKABLE void saveToSettings();
    Q_INVOKABLE void loadFromSettings();

signals:
    void buttonListChanged();
};
