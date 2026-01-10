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

/// Stream selector for switching between manually configured video streams
/// Displays stream buttons and volume control
Column {
    id: _root
    spacing: ScreenTools.defaultFontPixelHeight * 0.25

    // Only show when:
    // - At least one stream exists
    // - NOT using MAVLink auto-configured streams
    visible: QGroundControl.settingsManager.videoSettings.streamConfigurations.count > 0 &&
             !QGroundControl.videoManager.autoStreamConfigured

    Repeater {
        model: QGroundControl.settingsManager.videoSettings.streamConfigurations

        Rectangle {
            width: ScreenTools.defaultFontPixelWidth * 20
            height: ScreenTools.defaultFontPixelHeight * 2
            radius: ScreenTools.defaultFontPixelHeight * 0.25
            color: isCurrentStream ? "#CC000000" : "#80000000"  // More opaque when selected
            opacity: 0.95
            border.color: isCurrentStream ? qgcPal.colorGreen : "#FFFFFF"
            border.width: isCurrentStream ? 2 : 1
            visible: object.enabled

            property bool isCurrentStream: QGroundControl.videoManager.currentManualStreamIndex === index

            QGCLabel {
                anchors.centerIn: parent
                text: object.name
                color: parent.isCurrentStream ? qgcPal.colorGreen : "white"
                font.bold: parent.isCurrentStream
                elide: Text.ElideRight
                width: parent.width - ScreenTools.defaultFontPixelWidth
                horizontalAlignment: Text.AlignHCenter
            }

            QGCMouseArea {
                fillItem: parent
                onClicked: {
                    if (!parent.isCurrentStream) {
                        QGroundControl.videoManager.switchToStream(index)
                    }
                }
            }

            // Visual feedback on hover
            Rectangle {
                anchors.fill: parent
                color: "white"
                opacity: parent.isCurrentStream ? 0 : (mouseArea.containsMouse ? 0.1 : 0)
                radius: parent.radius
            }

            MouseArea {
                id: mouseArea
                anchors.fill: parent
                hoverEnabled: true
                onClicked: {
                    if (!parent.isCurrentStream) {
                        QGroundControl.videoManager.switchToStream(index)
                    }
                }
            }
        }
    }

    // Volume control - only shown when audio is available
    Rectangle {
        width: ScreenTools.defaultFontPixelWidth * 20
        height: ScreenTools.defaultFontPixelHeight * 3
        radius: ScreenTools.defaultFontPixelHeight * 0.25
        color: "#CC000000"
        opacity: 0.95
        border.color: "#FFFFFF"
        border.width: 1
        visible: QGroundControl.videoManager.audioAvailable

        Column {
            anchors.fill: parent
            anchors.margins: ScreenTools.defaultFontPixelHeight * 0.25
            spacing: ScreenTools.defaultFontPixelHeight * 0.1

            QGCLabel {
                width: parent.width
                text: qsTr("Volume")
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
                    value: QGroundControl.settingsManager.videoSettings.audioVolume.rawValue
                    onValueChanged: {
                        if (value !== QGroundControl.settingsManager.videoSettings.audioVolume.rawValue) {
                            QGroundControl.settingsManager.videoSettings.audioVolume.rawValue = value
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

    // Show error feedback when stream switch fails
    Connections {
        target: QGroundControl.videoManager
        function onStreamSwitchFailed(reason) {
            mainWindow.showMessageDialog(qsTr("Stream Switch Failed"), reason)
        }
    }
}
