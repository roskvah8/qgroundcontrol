/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

/// Dynamic servo button controls loaded from ServoButtonConfigurationList.
/// Buttons appear in Fly View and send MAV_CMD_DO_SET_SERVO commands.
ColumnLayout {
    id:         _root
    spacing:    ScreenTools.defaultFontPixelHeight / 4
    visible:    _activeVehicle && _servoButtons.count > 0

    property var  _activeVehicle: globals.activeVehicle
    property var  _servoButtons:  QGroundControl.settingsManager.servoButtonConfigurations
    property bool _isManualMode:  _activeVehicle && _activeVehicle.initialConnectComplete
        && _activeVehicle.stabilizedFlightMode !== ""
        && _activeVehicle.flightMode === _activeVehicle.stabilizedFlightMode

    readonly property int _cmdDoSetServo:   183   // MAV_CMD_DO_SET_SERVO
    readonly property int _compIdAutopilot: 1     // MAV_COMP_ID_AUTOPILOT1

    function _sendServoCommand(channel, pwm) {
        if (_activeVehicle) {
            _activeVehicle.sendCommand(_compIdAutopilot, _cmdDoSetServo, true, channel, pwm)
        }
    }

    Repeater {
        model: _servoButtons

        // Each delegate is a self-contained servo button with its own state and timer
        Item {
            id:                     delegate
            Layout.alignment:       Qt.AlignRight
            width:                  buttonLoader.width
            height:                 buttonLoader.height

            required property var   model
            required property int   index

            property var  _config:      _servoButtons.getButton(index)
            property bool _activated:   false
            property bool _toggled:     false
            property int  _remaining:   0

            // Determine behavior mode
            property bool _useDelay:    _config ? _config.holdTime > 0 : false
            property bool _hasCooldown: _config ? _config.activeTime > 0 : false
            property bool _isPushHold:  !_useDelay && !_hasCooldown

            property bool _buttonEnabled: {
                if (!_root._activeVehicle) return false
                if (!_root._isManualMode) return false
                if (_config && _config.enabledWhenArmed && !_root._activeVehicle.armed) return false
                if (_activated) return false
                return true
            }

            property string _buttonText: {
                if (!_config) return ""
                if (_activated && _remaining > 0) {
                    return qsTr("%1 (%2s)").arg(_config.name).arg(_remaining)
                }
                if (_toggled) {
                    return qsTr("%1 (ON)").arg(_config.name)
                }
                return _config.name
            }

            Timer {
                id:       activeTimer
                interval: 1000
                repeat:   true
                onTriggered: {
                    delegate._remaining--
                    if (delegate._remaining <= 0) {
                        activeTimer.stop()
                        if (delegate._config) {
                            _root._sendServoCommand(delegate._config.servoChannel, delegate._config.defaultPwm)
                        }
                        delegate._activated = false
                    }
                }
            }

            function _activate() {
                if (!_config) return
                _root._sendServoCommand(_config.servoChannel, _config.activePwm)
                if (_hasCooldown) {
                    _activated = true
                    _remaining = _config.activeTime
                    activeTimer.start()
                } else {
                    // Toggle mode (holdTime > 0, activeTime = 0)
                    _toggled = true
                }
            }

            function _deactivateToggle() {
                if (!_config) return
                _root._sendServoCommand(_config.servoChannel, _config.defaultPwm)
                _toggled = false
            }

            Loader {
                id:              buttonLoader
                sourceComponent: delegate._isPushHold ? pushHoldComponent :
                                 delegate._useDelay ? delayButtonComponent :
                                 clickButtonComponent
            }

            // Mode 1: QGCDelayButton for holdTime > 0 (with activeTime or toggle)
            Component {
                id: delayButtonComponent

                QGCDelayButton {
                    text:         delegate._buttonText
                    enabled:      delegate._toggled || delegate._buttonEnabled
                    defaultDelay: delegate._config ? delegate._config.holdTime * 1000 : 500
                    width:        ScreenTools.defaultFontPixelWidth * 20

                    onActivated: {
                        if (delegate._toggled) {
                            delegate._deactivateToggle()
                        } else {
                            delegate._activate()
                        }
                    }
                }
            }

            // Mode 2: QGCButton for click-to-activate (holdTime = 0, activeTime > 0)
            Component {
                id: clickButtonComponent

                QGCButton {
                    text:    delegate._buttonText
                    enabled: delegate._buttonEnabled
                    width:   ScreenTools.defaultFontPixelWidth * 20

                    onClicked: delegate._activate()
                }
            }

            // Mode 3: Push-to-hold (holdTime = 0, activeTime = 0)
            Component {
                id: pushHoldComponent

                QGCButton {
                    text:    delegate._config ? delegate._config.name : ""
                    enabled: delegate._buttonEnabled
                    width:   ScreenTools.defaultFontPixelWidth * 20

                    onPressedChanged: {
                        if (!delegate._config) return
                        if (pressed) {
                            _root._sendServoCommand(delegate._config.servoChannel, delegate._config.activePwm)
                        } else {
                            _root._sendServoCommand(delegate._config.servoChannel, delegate._config.defaultPwm)
                        }
                    }
                }
            }
        }
    }
}
