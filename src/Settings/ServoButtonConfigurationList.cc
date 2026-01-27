/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "ServoButtonConfigurationList.h"
#include "ServoButtonConfiguration.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QSettings>

QGC_LOGGING_CATEGORY(ServoButtonConfigurationListLog, "qgc.settings.servobuttonconfigurationlist")

static constexpr const char *kButtonListKey = "ServoButtons/buttonList";

ServoButtonConfigurationList::ServoButtonConfigurationList(QObject *parent)
    : QmlObjectListModel(parent)
{
}

bool ServoButtonConfigurationList::isChannelDuplicate(int servoChannel, int excludeIndex) const
{
    for (int i = 0; i < count(); i++) {
        if (i == excludeIndex) {
            continue;
        }
        auto *config = qobject_cast<ServoButtonConfiguration *>(const_cast<ServoButtonConfigurationList *>(this)->get(i));
        if (config && config->servoChannel() == servoChannel) {
            return true;
        }
    }
    return false;
}

bool ServoButtonConfigurationList::addButton(const QString &name, int servoChannel, int defaultPwm,
                                             int activePwm, int holdTime, int activeTime, bool enabledWhenArmed)
{
    if (name.trimmed().isEmpty()) {
        qCWarning(ServoButtonConfigurationListLog) << "Cannot add button with empty name";
        return false;
    }

    if (isChannelDuplicate(servoChannel)) {
        qCWarning(ServoButtonConfigurationListLog) << "Cannot add button: servo channel" << servoChannel << "already in use";
        return false;
    }

    auto *config = new ServoButtonConfiguration(name, servoChannel, defaultPwm, activePwm,
                                                holdTime, activeTime, enabledWhenArmed, this);
    append(config);

    saveToSettings();
    emit buttonListChanged();

    qCDebug(ServoButtonConfigurationListLog) << "Added servo button:" << name << "channel:" << servoChannel;
    return true;
}

void ServoButtonConfigurationList::removeButton(int index)
{
    if (index < 0 || index >= count()) {
        qCWarning(ServoButtonConfigurationListLog) << "Invalid button index:" << index;
        return;
    }

    auto *config = qobject_cast<ServoButtonConfiguration *>(get(index));
    const QString name = config ? config->name() : QStringLiteral("unknown");

    removeAt(index);

    saveToSettings();
    emit buttonListChanged();

    qCDebug(ServoButtonConfigurationListLog) << "Removed servo button:" << name << "at index" << index;
}

ServoButtonConfiguration *ServoButtonConfigurationList::getButton(int index)
{
    if (index < 0 || index >= count()) {
        return nullptr;
    }
    return qobject_cast<ServoButtonConfiguration *>(get(index));
}

bool ServoButtonConfigurationList::updateButton(int index, const QString &name, int servoChannel,
                                                int defaultPwm, int activePwm, int holdTime,
                                                int activeTime, bool enabledWhenArmed)
{
    if (index < 0 || index >= count()) {
        qCWarning(ServoButtonConfigurationListLog) << "Invalid button index for update:" << index;
        return false;
    }

    if (name.trimmed().isEmpty()) {
        qCWarning(ServoButtonConfigurationListLog) << "Cannot update button with empty name";
        return false;
    }

    if (isChannelDuplicate(servoChannel, index)) {
        qCWarning(ServoButtonConfigurationListLog) << "Cannot update button: servo channel" << servoChannel << "already in use";
        return false;
    }

    auto *config = qobject_cast<ServoButtonConfiguration *>(get(index));
    if (!config) {
        return false;
    }

    config->setName(name);
    config->setServoChannel(servoChannel);
    config->setDefaultPwm(defaultPwm);
    config->setActivePwm(activePwm);
    config->setHoldTime(holdTime);
    config->setActiveTime(activeTime);
    config->setEnabledWhenArmed(enabledWhenArmed);

    saveToSettings();
    emit buttonListChanged();

    qCDebug(ServoButtonConfigurationListLog) << "Updated servo button:" << name << "channel:" << servoChannel;
    return true;
}

void ServoButtonConfigurationList::saveToSettings()
{
    QJsonArray jsonArray;

    for (int i = 0; i < count(); i++) {
        auto *config = qobject_cast<ServoButtonConfiguration *>(get(i));
        if (config) {
            jsonArray.append(config->toJson());
        }
    }

    QJsonDocument doc(jsonArray);
    const QString jsonString = doc.toJson(QJsonDocument::Compact);

    QSettings settings;
    settings.setValue(kButtonListKey, jsonString);

    qCDebug(ServoButtonConfigurationListLog) << "Saved" << count() << "servo button configurations to settings";
}

void ServoButtonConfigurationList::loadFromSettings()
{
    QSettings settings;
    const QString jsonString = settings.value(kButtonListKey).toString();

    if (jsonString.isEmpty()) {
        qCDebug(ServoButtonConfigurationListLog) << "No saved servo button configurations found";
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(jsonString.toUtf8());
    if (!doc.isArray()) {
        qCWarning(ServoButtonConfigurationListLog) << "Invalid servo button configuration JSON (not an array)";
        return;
    }

    beginResetModel();
    clear();

    const QJsonArray jsonArray = doc.array();
    for (const QJsonValue &value : jsonArray) {
        if (!value.isObject()) {
            qCWarning(ServoButtonConfigurationListLog) << "Skipping invalid servo button configuration (not an object)";
            continue;
        }

        ServoButtonConfiguration *config = ServoButtonConfiguration::fromJson(value.toObject(), this);
        if (config) {
            // Skip duplicate channels during load
            if (isChannelDuplicate(config->servoChannel())) {
                qCWarning(ServoButtonConfigurationListLog) << "Skipping duplicate servo channel" << config->servoChannel()
                                                           << "for button" << config->name();
                delete config;
                continue;
            }
            append(config);
        }
    }

    endResetModel();

    qCDebug(ServoButtonConfigurationListLog) << "Loaded" << count() << "servo button configurations";
    emit buttonListChanged();
}
