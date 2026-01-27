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

Q_DECLARE_LOGGING_CATEGORY(ServoButtonConfigurationLog)

/// Represents a single servo button configuration
/// Stores button name, servo channel, PWM values, timing, and behavior settings
class ServoButtonConfiguration : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("ServoButtonConfiguration should not be created in QML")

    Q_PROPERTY(QString name            READ name            WRITE setName            NOTIFY nameChanged)
    Q_PROPERTY(int     servoChannel    READ servoChannel    WRITE setServoChannel    NOTIFY servoChannelChanged)
    Q_PROPERTY(int     defaultPwm      READ defaultPwm      WRITE setDefaultPwm      NOTIFY defaultPwmChanged)
    Q_PROPERTY(int     activePwm       READ activePwm       WRITE setActivePwm       NOTIFY activePwmChanged)
    Q_PROPERTY(int     holdTime        READ holdTime        WRITE setHoldTime        NOTIFY holdTimeChanged)
    Q_PROPERTY(int     activeTime      READ activeTime      WRITE setActiveTime      NOTIFY activeTimeChanged)
    Q_PROPERTY(bool    enabledWhenArmed READ enabledWhenArmed WRITE setEnabledWhenArmed NOTIFY enabledWhenArmedChanged)

public:
    explicit ServoButtonConfiguration(QObject *parent = nullptr);
    ServoButtonConfiguration(const QString &name, int servoChannel, int defaultPwm, int activePwm,
                             int holdTime, int activeTime, bool enabledWhenArmed, QObject *parent = nullptr);
    ~ServoButtonConfiguration() override = default;

    // Validation constants
    static constexpr int kMinServoChannel = 1;
    static constexpr int kMaxServoChannel = 16;
    static constexpr int kMinPwm = 800;
    static constexpr int kMaxPwm = 2200;

    // Property accessors
    QString name() const { return _name; }
    int servoChannel() const { return _servoChannel; }
    int defaultPwm() const { return _defaultPwm; }
    int activePwm() const { return _activePwm; }
    int holdTime() const { return _holdTime; }
    int activeTime() const { return _activeTime; }
    bool enabledWhenArmed() const { return _enabledWhenArmed; }

    void setName(const QString &name);
    void setServoChannel(int servoChannel);
    void setDefaultPwm(int defaultPwm);
    void setActivePwm(int activePwm);
    void setHoldTime(int holdTime);
    void setActiveTime(int activeTime);
    void setEnabledWhenArmed(bool enabledWhenArmed);

    // JSON serialization
    [[nodiscard]] QJsonObject toJson() const;
    static ServoButtonConfiguration *fromJson(const QJsonObject &json, QObject *parent = nullptr);

signals:
    void nameChanged(QString name);
    void servoChannelChanged(int servoChannel);
    void defaultPwmChanged(int defaultPwm);
    void activePwmChanged(int activePwm);
    void holdTimeChanged(int holdTime);
    void activeTimeChanged(int activeTime);
    void enabledWhenArmedChanged(bool enabledWhenArmed);

private:
    QString _name;
    int _servoChannel = 1;
    int _defaultPwm = 1000;
    int _activePwm = 2000;
    int _holdTime = 0;
    int _activeTime = 5;
    bool _enabledWhenArmed = false;
};
