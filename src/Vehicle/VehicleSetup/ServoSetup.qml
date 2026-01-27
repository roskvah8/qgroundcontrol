/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

Item {
    id: _root

    property var _servoButtons: QGroundControl.settingsManager.servoButtonConfigurations

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    QGCFlickable {
        anchors.fill:       parent
        anchors.margins:    ScreenTools.defaultFontPixelWidth
        contentHeight:      mainColumn.height
        contentWidth:       mainColumn.width
        flickableDirection: Flickable.VerticalFlick
        clip:               true

        ColumnLayout {
            id:         mainColumn
            width:      Math.min(_root.width - ScreenTools.defaultFontPixelWidth * 2,
                                 ScreenTools.defaultFontPixelWidth * 80)
            spacing:    ScreenTools.defaultFontPixelHeight

            // Header
            RowLayout {
                Layout.fillWidth: true
                spacing:          ScreenTools.defaultFontPixelWidth * 2

                QGCLabel {
                    text:               qsTr("Servo Button Configuration")
                    font.pointSize:     ScreenTools.mediumFontPointSize
                    font.bold:          true
                    Layout.fillWidth:   true
                }

                QGCButton {
                    text:       qsTr("Add Servo Button")
                    onClicked: {
                        // Find the next available channel
                        var channel = 1
                        for (var i = 1; i <= 16; i++) {
                            if (!_servoButtons.isChannelDuplicate(i)) {
                                channel = i
                                break
                            }
                        }
                        _servoButtons.addButton(
                            qsTr("Servo %1").arg(channel),
                            channel, 1000, 2000, 0, 5, false
                        )
                    }
                }
            }

            QGCLabel {
                Layout.fillWidth:   true
                wrapMode:           Text.WordWrap
                font.pointSize:     ScreenTools.smallFontPointSize
                text:               qsTr("Configure servo buttons that appear on the Fly View. Each button sends a MAV_CMD_DO_SET_SERVO command to the specified channel.")
                visible:            _servoButtons.count === 0
            }

            QGCLabel {
                Layout.fillWidth:   true
                wrapMode:           Text.WordWrap
                text:               qsTr("No servo buttons configured. Click 'Add Servo Button' to create one.")
                visible:            _servoButtons.count === 0
                horizontalAlignment: Text.AlignHCenter
                font.pointSize:     ScreenTools.defaultFontPointSize
            }

            // Button list
            Repeater {
                model: _servoButtons

                Rectangle {
                    id:                 buttonCard
                    Layout.fillWidth:   true
                    implicitHeight:     cardColumn.implicitHeight + ScreenTools.defaultFontPixelHeight
                    color:              qgcPal.windowShade
                    radius:             ScreenTools.defaultFontPixelHeight / 2
                    border.color:       _channelDuplicate ? "red" : qgcPal.groupBorder
                    border.width:       1

                    required property var model
                    required property int index

                    property var _config: _servoButtons.getButton(index)
                    property bool _channelDuplicate: _config ? _servoButtons.isChannelDuplicate(_config.servoChannel, index) : false

                    function _saveConfig() {
                        if (_config) {
                            _servoButtons.updateButton(
                                index,
                                nameField.text,
                                parseInt(channelField.text) || 1,
                                parseInt(defaultPwmField.text) || 1000,
                                parseInt(activePwmField.text) || 2000,
                                parseInt(holdTimeField.text) || 0,
                                parseInt(activeTimeField.text) || 0,
                                armedCheckbox.checked
                            )
                        }
                    }

                    ColumnLayout {
                        id:             cardColumn
                        anchors {
                            left:       parent.left
                            right:      parent.right
                            top:        parent.top
                            margins:    ScreenTools.defaultFontPixelHeight / 2
                        }
                        spacing:        ScreenTools.defaultFontPixelHeight / 2

                        // Card header
                        RowLayout {
                            Layout.fillWidth: true

                            QGCLabel {
                                text:               _config ? _config.name : ""
                                font.bold:          true
                                font.pointSize:     ScreenTools.defaultFontPointSize + 1
                                Layout.fillWidth:   true
                            }

                            QGCLabel {
                                text:               qsTr("Channel %1").arg(_config ? _config.servoChannel : "")
                                font.pointSize:     ScreenTools.smallFontPointSize
                                color:              buttonCard._channelDuplicate ? "red" : qgcPal.text
                            }

                            QGCButton {
                                text:       qsTr("Remove")
                                onClicked:  _servoButtons.removeButton(index)
                            }
                        }

                        // Duplicate channel warning
                        QGCLabel {
                            Layout.fillWidth:   true
                            text:               qsTr("Warning: Another button already uses servo channel %1").arg(_config ? _config.servoChannel : "")
                            color:              "red"
                            wrapMode:           Text.WordWrap
                            visible:            buttonCard._channelDuplicate
                        }

                        // Form fields
                        GridLayout {
                            Layout.fillWidth:   true
                            columns:            4
                            columnSpacing:      ScreenTools.defaultFontPixelWidth * 2
                            rowSpacing:         ScreenTools.defaultFontPixelHeight / 2

                            // Row 1: Name + Channel
                            QGCLabel { text: qsTr("Name") }
                            QGCTextField {
                                id:                     nameField
                                Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 20
                                text:                   _config ? _config.name : ""
                                onEditingFinished:      buttonCard._saveConfig()
                            }

                            QGCLabel { text: qsTr("Servo Channel (1-16)") }
                            QGCTextField {
                                id:                     channelField
                                Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 8
                                text:                   _config ? _config.servoChannel : ""
                                inputMethodHints:       Qt.ImhDigitsOnly
                                validator:              IntValidator { bottom: 1; top: 16 }
                                onEditingFinished:      buttonCard._saveConfig()
                            }

                            // Row 2: Default PWM + Active PWM
                            QGCLabel { text: qsTr("Default PWM (800-2200)") }
                            QGCTextField {
                                id:                     defaultPwmField
                                Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 8
                                text:                   _config ? _config.defaultPwm : ""
                                inputMethodHints:       Qt.ImhDigitsOnly
                                validator:              IntValidator { bottom: 800; top: 2200 }
                                onEditingFinished:      buttonCard._saveConfig()
                            }

                            QGCLabel { text: qsTr("Active PWM (800-2200)") }
                            QGCTextField {
                                id:                     activePwmField
                                Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 8
                                text:                   _config ? _config.activePwm : ""
                                inputMethodHints:       Qt.ImhDigitsOnly
                                validator:              IntValidator { bottom: 800; top: 2200 }
                                onEditingFinished:      buttonCard._saveConfig()
                            }

                            // Row 3: Hold Time + Active Time
                            QGCLabel { text: qsTr("Hold Time (s)") }
                            QGCTextField {
                                id:                     holdTimeField
                                Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 8
                                text:                   _config ? _config.holdTime : ""
                                inputMethodHints:       Qt.ImhDigitsOnly
                                validator:              IntValidator { bottom: 0; top: 60 }
                                onEditingFinished:      buttonCard._saveConfig()
                            }

                            QGCLabel { text: qsTr("Active (s)") }
                            QGCTextField {
                                id:                     activeTimeField
                                Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 8
                                text:                   _config ? _config.activeTime : ""
                                inputMethodHints:       Qt.ImhDigitsOnly
                                validator:              IntValidator { bottom: 0; top: 300 }
                                onEditingFinished:      buttonCard._saveConfig()
                            }

                            // Row 4: Enabled When Armed
                            QGCLabel { text: qsTr("Enabled When Armed") }
                            QGCCheckBox {
                                id:         armedCheckbox
                                checked:    _config ? _config.enabledWhenArmed : false
                                onClicked:  buttonCard._saveConfig()
                            }
                        }

                        // Behavior description
                        QGCLabel {
                            Layout.fillWidth:   true
                            wrapMode:           Text.WordWrap
                            font.pointSize:     ScreenTools.smallFontPointSize
                            color:              qgcPal.text
                            text: {
                                if (!_config) return ""
                                var holdTime = parseInt(holdTimeField.text) || 0
                                var activeTime = parseInt(activeTimeField.text) || 0
                                if (holdTime > 0 && activeTime > 0) {
                                    return qsTr("Behavior: Hold button for %1s to confirm, then servo activates for %2s").arg(holdTime).arg(activeTime)
                                } else if (holdTime === 0 && activeTime > 0) {
                                    return qsTr("Behavior: Click to activate servo for %1s").arg(activeTime)
                                } else if (holdTime > 0 && activeTime === 0) {
                                    return qsTr("Behavior: Hold button for %1s to confirm, then toggle on/off").arg(holdTime)
                                } else {
                                    return qsTr("Behavior: Press and hold to activate, release to deactivate")
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
