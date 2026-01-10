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
import QGroundControl.FactControls
import QGroundControl.Controls

SettingsPage {
    id: root

    property var  _voiceStreamingManager:      QGroundControl.voiceStreamingManager
    property var  _settingsManager:            QGroundControl.settingsManager
    property var  _voiceStreamingSettings:     _settingsManager.voiceStreamingSettings
    property Fact _voiceStreamingEnabled:      _voiceStreamingSettings.voiceStreamingEnabled
    property Fact _voicePeerAddress:           _voiceStreamingSettings.voicePeerAddress
    property Fact _voiceVolume:                _voiceStreamingSettings.voiceVolume

    SettingsGroupLayout {
        Layout.fillWidth: true
        heading: qsTr("Voice Streaming")
        headingDescription: qsTr("Configure live voice communication via WebRTC")

        FactCheckBoxSlider {
            Layout.fillWidth: true
            text: qsTr("Voice Streaming Enabled")
            fact: _voiceStreamingEnabled
            visible: fact.visible
            // Auto-start/stop handled by C++ settings change handler
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: ScreenTools.defaultFontPixelHeight * 0.25
            enabled: _voiceStreamingEnabled.rawValue

            QGCLabel {
                text: qsTr("Receiver Address")
            }

            QGCTextField {
                id: peerAddressField
                Layout.fillWidth: true
                text: _voicePeerAddress.rawValue
                placeholderText: qsTr("http://192.168.1.100:8080")

                onEditingFinished: {
                    _voicePeerAddress.rawValue = text
                    // Auto-connect handled by C++ settings change handler
                }
            }

            QGCLabel {
                Layout.fillWidth: true
                text: qsTr("WebRTC receiver address (HTTP endpoint for signaling)")
                font.pointSize: ScreenTools.smallFontPointSize
                wrapMode: Text.WordWrap
            }
        }

        LabelledFactTextField {
            Layout.fillWidth: true
            label: qsTr("Volume")
            fact: _voiceVolume
            visible: fact.visible
        }
    }

    // Update volume when streaming starts
    Connections {
        target: _voiceStreamingManager

        function onStreamingChanged() {
            if (_voiceStreamingManager && _voiceStreamingManager.streaming) {
                _voiceStreamingManager.setVoiceVolume(_voiceVolume.rawValue)
            }
        }
    }

    // Update volume when it changes in settings
    Connections {
        target: _voiceVolume

        function onRawValueChanged() {
            if (_voiceStreamingManager && _voiceStreamingManager.streaming) {
                _voiceStreamingManager.setVoiceVolume(_voiceVolume.rawValue)
            }
        }
    }
}
