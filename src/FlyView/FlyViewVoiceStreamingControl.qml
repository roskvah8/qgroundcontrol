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

/// Voice streaming control widget for push-to-talk functionality
/// Displays connection status and push-to-talk button when voice streaming is enabled
FocusScope {
    id: _root
    width: childrenRect.width
    height: childrenRect.height

    // Grab focus when visible to capture spacebar events
    focus: visible
    activeFocusOnTab: false

    // Voice streaming properties
    property var _voiceStreamingManager: QGroundControl.voiceStreamingManager
    property bool _voiceStreamingEnabled: {
        var voiceStreamingSettings = QGroundControl.settingsManager.voiceStreamingSettings
        return voiceStreamingSettings ? voiceStreamingSettings.voiceStreamingEnabled.rawValue : false
    }

    visible: _voiceStreamingManager && _voiceStreamingEnabled

    // Keyboard handler for push-to-talk (hold T key to transmit)
    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_T && !event.isAutoRepeat && event.modifiers === Qt.NoModifier) {
            if (_voiceStreamingManager && _voiceStreamingManager.streaming && _voiceStreamingManager.connected) {
                event.accepted = true
                _voiceStreamingManager.pushToTalkActive = true
            }
        }
    }

    Keys.onReleased: (event) => {
        if (event.key === Qt.Key_T && !event.isAutoRepeat) {
            if (_voiceStreamingManager && _voiceStreamingManager.streaming && _voiceStreamingManager.connected) {
                event.accepted = true
                _voiceStreamingManager.pushToTalkActive = false
            }
        }
    }

    Column {
        spacing: ScreenTools.defaultFontPixelHeight * 0.25

    // Update volume when streaming state changes
    Connections {
        target: _voiceStreamingManager

        function onStreamingChanged() {
            if (_voiceStreamingManager && _voiceStreamingManager.streaming) {
                var voiceStreamingSettings = QGroundControl.settingsManager.voiceStreamingSettings
                if (voiceStreamingSettings) {
                    _voiceStreamingManager.setVoiceVolume(voiceStreamingSettings.voiceVolume.rawValue)
                }
            }
        }
    }

    // Push-to-Talk button
    Rectangle {
        id: pttButton
        width: ScreenTools.defaultFontPixelWidth * 20
        height: ScreenTools.defaultFontPixelHeight * 3.5
        radius: ScreenTools.defaultFontPixelHeight * 0.25
        color: _root._voiceStreamingManager && _root._voiceStreamingManager.pushToTalkActive ? qgcPal.colorRed : "#CC000000"
        opacity: 0.95
        border.color: _root._voiceStreamingManager && _root._voiceStreamingManager.pushToTalkActive ? qgcPal.colorRed : "#FFFFFF"
        border.width: _root._voiceStreamingManager && _root._voiceStreamingManager.pushToTalkActive ? 2 : 1

        Column {
            anchors.fill: parent
            anchors.margins: ScreenTools.defaultFontPixelHeight * 0.25
            spacing: ScreenTools.defaultFontPixelHeight * 0.1

            // Connection status indicator
            Row {
                width: parent.width
                spacing: ScreenTools.defaultFontPixelWidth * 0.5

                Rectangle {
                    width: ScreenTools.defaultFontPixelHeight * 0.6
                    height: width
                    radius: width / 2
                    color: {
                        if (!_root._voiceStreamingManager || !_root._voiceStreamingManager.streaming) {
                            return qgcPal.colorGrey
                        } else if (_root._voiceStreamingManager.connected) {
                            return qgcPal.colorGreen
                        } else {
                            return qgcPal.colorOrange
                        }
                    }
                    anchors.verticalCenter: parent.verticalCenter
                }

                QGCLabel {
                    text: {
                        if (!_root._voiceStreamingManager || !_root._voiceStreamingManager.streaming) {
                            return qsTr("Push to Talk")
                        } else if (_root._voiceStreamingManager.pushToTalkActive) {
                            return qsTr("🔴 TRANSMITTING")
                        } else if (_root._voiceStreamingManager.connected) {
                            return qsTr("Ready (Hold)")
                        } else {
                            return qsTr("Connecting...")
                        }
                    }
                    color: "white"
                    font.pointSize: ScreenTools.smallFontPointSize
                    font.bold: _root._voiceStreamingManager && _root._voiceStreamingManager.pushToTalkActive
                    elide: Text.ElideRight
                    width: parent.width - ScreenTools.defaultFontPixelWidth * 2
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            // T key hint
            QGCLabel {
                width: parent.width
                text: qsTr("Hold T")
                color: "#AAAAAA"
                font.pointSize: ScreenTools.smallFontPointSize * 0.8
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideRight
            }
        }

        // Press and hold to transmit
        MouseArea {
            anchors.fill: parent
            enabled: _root._voiceStreamingManager && _root._voiceStreamingManager.streaming && _root._voiceStreamingManager.connected

            onPressedChanged: {
                if (_root._voiceStreamingManager && _root._voiceStreamingManager.streaming && _root._voiceStreamingManager.connected) {
                    _root._voiceStreamingManager.pushToTalkActive = pressed
                }
            }
        }

        // Visual feedback on hover
        Rectangle {
            anchors.fill: parent
            color: "white"
            opacity: pttHoverArea.containsMouse ? 0.1 : 0
            radius: parent.radius
        }

        MouseArea {
            id: pttHoverArea
            anchors.fill: parent
            hoverEnabled: true
            enabled: _root._voiceStreamingManager && _root._voiceStreamingManager.streaming && _root._voiceStreamingManager.connected

            onPressedChanged: {
                if (_root._voiceStreamingManager && _root._voiceStreamingManager.streaming && _root._voiceStreamingManager.connected) {
                    _root._voiceStreamingManager.pushToTalkActive = pressed
                }
            }
        }
    }

    // Voice volume control
    Rectangle {
        width: ScreenTools.defaultFontPixelWidth * 20
        height: ScreenTools.defaultFontPixelHeight * 3
        radius: ScreenTools.defaultFontPixelHeight * 0.25
        color: "#CC000000"
        opacity: 0.95
        border.color: "#FFFFFF"
        border.width: 1

        Column {
            anchors.fill: parent
            anchors.margins: ScreenTools.defaultFontPixelHeight * 0.25
            spacing: ScreenTools.defaultFontPixelHeight * 0.1

            QGCLabel {
                width: parent.width
                text: qsTr("Voice Volume")
                color: "white"
                font.pointSize: ScreenTools.smallFontPointSize
                horizontalAlignment: Text.AlignHCenter
            }

            RowLayout {
                width: parent.width
                spacing: ScreenTools.defaultFontPixelWidth * 0.5

                QGCColoredImage {
                    width: ScreenTools.defaultFontPixelHeight * 0.8
                    height: width
                    sourceSize.height: height
                    source: "/InstrumentValueIcons/volume-mute.svg"
                    color: "white"
                    fillMode: Image.PreserveAspectFit
                }

                QGCSlider {
                    Layout.fillWidth: true
                    from: 0
                    to: 100
                    value: QGroundControl.settingsManager.voiceStreamingSettings.voiceVolume.rawValue
                    onValueChanged: {
                        if (value !== QGroundControl.settingsManager.voiceStreamingSettings.voiceVolume.rawValue) {
                            QGroundControl.settingsManager.voiceStreamingSettings.voiceVolume.rawValue = value
                            // Only apply volume if streaming is active
                            if (_root._voiceStreamingManager && _root._voiceStreamingManager.streaming) {
                                _root._voiceStreamingManager.setVoiceVolume(value)
                            }
                        }
                    }
                }

                QGCColoredImage {
                    width: ScreenTools.defaultFontPixelHeight * 0.8
                    height: width
                    sourceSize.height: height
                    source: "/InstrumentValueIcons/volume-up.svg"
                    color: "white"
                    fillMode: Image.PreserveAspectFit
                }
            }
        }
    }

    // Show voice streaming errors
    Connections {
        target: _voiceStreamingManager

        function onErrorOccurred(error) {
            mainWindow.showMessageDialog(qsTr("Voice Streaming Error"), error)
        }
    }
    }  // Column
}
