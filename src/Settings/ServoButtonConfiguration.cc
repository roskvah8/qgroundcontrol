/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "ServoButtonConfiguration.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QJsonDocument>

#include <algorithm>

QGC_LOGGING_CATEGORY(ServoButtonConfigurationLog, "qgc.settings.servobuttonconfiguration")

ServoButtonConfiguration::ServoButtonConfiguration(QObject *parent)
    : QObject(parent)
{
}

ServoButtonConfiguration::ServoButtonConfiguration(const QString &name, int servoChannel, int defaultPwm,
                                                   int activePwm, int holdTime, int activeTime,
                                                   bool enabledWhenArmed, QObject *parent)
    : QObject(parent)
    , _name(name)
    , _servoChannel(std::clamp(servoChannel, kMinServoChannel, kMaxServoChannel))
    , _defaultPwm(std::clamp(defaultPwm, kMinPwm, kMaxPwm))
    , _activePwm(std::clamp(activePwm, kMinPwm, kMaxPwm))
    , _holdTime(std::max(0, holdTime))
    , _activeTime(std::max(0, activeTime))
    , _enabledWhenArmed(enabledWhenArmed)
{
}

void ServoButtonConfiguration::setName(const QString &name)
{
    if (_name != name) {
        _name = name;
        emit nameChanged(_name);
    }
}

void ServoButtonConfiguration::setServoChannel(int servoChannel)
{
    servoChannel = std::clamp(servoChannel, kMinServoChannel, kMaxServoChannel);
    if (_servoChannel != servoChannel) {
        _servoChannel = servoChannel;
        emit servoChannelChanged(_servoChannel);
    }
}

void ServoButtonConfiguration::setDefaultPwm(int defaultPwm)
{
    defaultPwm = std::clamp(defaultPwm, kMinPwm, kMaxPwm);
    if (_defaultPwm != defaultPwm) {
        _defaultPwm = defaultPwm;
        emit defaultPwmChanged(_defaultPwm);
    }
}

void ServoButtonConfiguration::setActivePwm(int activePwm)
{
    activePwm = std::clamp(activePwm, kMinPwm, kMaxPwm);
    if (_activePwm != activePwm) {
        _activePwm = activePwm;
        emit activePwmChanged(_activePwm);
    }
}

void ServoButtonConfiguration::setHoldTime(int holdTime)
{
    holdTime = std::max(0, holdTime);
    if (_holdTime != holdTime) {
        _holdTime = holdTime;
        emit holdTimeChanged(_holdTime);
    }
}

void ServoButtonConfiguration::setActiveTime(int activeTime)
{
    activeTime = std::max(0, activeTime);
    if (_activeTime != activeTime) {
        _activeTime = activeTime;
        emit activeTimeChanged(_activeTime);
    }
}

void ServoButtonConfiguration::setEnabledWhenArmed(bool enabledWhenArmed)
{
    if (_enabledWhenArmed != enabledWhenArmed) {
        _enabledWhenArmed = enabledWhenArmed;
        emit enabledWhenArmedChanged(_enabledWhenArmed);
    }
}

QJsonObject ServoButtonConfiguration::toJson() const
{
    QJsonObject json;
    json["name"] = _name;
    json["servoChannel"] = _servoChannel;
    json["defaultPwm"] = _defaultPwm;
    json["activePwm"] = _activePwm;
    json["holdTime"] = _holdTime;
    json["activeTime"] = _activeTime;
    json["enabledWhenArmed"] = _enabledWhenArmed;
    return json;
}

ServoButtonConfiguration *ServoButtonConfiguration::fromJson(const QJsonObject &json, QObject *parent)
{
    if (!json.contains("name") || !json.contains("servoChannel")) {
        qCWarning(ServoButtonConfigurationLog) << "Invalid JSON: missing required fields";
        return nullptr;
    }

    const QString name = json["name"].toString();
    const int servoChannel = json["servoChannel"].toInt(1);
    const int defaultPwm = json["defaultPwm"].toInt(1000);
    const int activePwm = json["activePwm"].toInt(2000);
    const int holdTime = json["holdTime"].toInt(0);
    const int activeTime = json["activeTime"].toInt(5);
    const bool enabledWhenArmed = json["enabledWhenArmed"].toBool(false);

    if (name.trimmed().isEmpty()) {
        qCWarning(ServoButtonConfigurationLog) << "Invalid configuration: empty name";
        return nullptr;
    }

    return new ServoButtonConfiguration(name, servoChannel, defaultPwm, activePwm,
                                        holdTime, activeTime, enabledWhenArmed, parent);
}
