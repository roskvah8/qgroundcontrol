/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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
    property var    _settingsManager:            QGroundControl.settingsManager
    property var    _videoManager:              QGroundControl.videoManager
    property var    _videoSettings:             _settingsManager.videoSettings
    property bool   _videoAutoStreamConfig:     _videoManager.autoStreamConfigured

    SettingsGroupLayout {
        Layout.fillWidth:   true
        heading:            qsTr("Video Streams")
        headingDescription: _videoAutoStreamConfig ?
                           qsTr("MAVLink camera stream is automatically configured") :
                           qsTr("Manage your video stream configurations")
        visible:            !_videoAutoStreamConfig
        showDividers:       false

        // Stream list
        Repeater {
            model: _videoSettings.streamConfigurations

            RowLayout {
                Layout.fillWidth: true
                spacing: ScreenTools.defaultFontPixelWidth

                // Enabled indicator (colored circle)
                Rectangle {
                    width: ScreenTools.defaultFontPixelHeight * 0.6
                    height: width
                    radius: width / 2
                    color: object.enabled ? qgcPal.colorGreen : qgcPal.colorGrey
                    border.width: 1
                    border.color: qgcPal.text
                    Layout.alignment: Qt.AlignVCenter
                }

                // Stream name
                QGCLabel {
                    Layout.fillWidth: true
                    text: object.name
                    elide: Text.ElideRight
                }

                // Stream type
                QGCLabel {
                    text: object.type
                    font.pointSize: ScreenTools.smallFontPointSize
                    Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 18
                    elide: Text.ElideRight
                }

                // Move up button
                QGCColoredImage {
                    height: ScreenTools.minTouchPixels
                    width: height
                    sourceSize.height: height
                    fillMode: Image.PreserveAspectFit
                    mipmap: true
                    smooth: true
                    color: canMoveUp ? qgcPalMoveUp.text : qgcPal.windowShade
                    source: "/InstrumentValueIcons/arrow-thin-up.svg"
                    enabled: canMoveUp

                    property bool canMoveUp: _videoSettings.streamConfigurations.canMoveUp(index)

                    QGCPalette {
                        id: qgcPalMoveUp
                        colorGroupEnabled: parent.enabled
                    }

                    QGCMouseArea {
                        fillItem: parent
                        onClicked: _videoSettings.streamConfigurations.moveStreamUp(index)
                    }
                }

                // Move down button
                QGCColoredImage {
                    height: ScreenTools.minTouchPixels
                    width: height
                    sourceSize.height: height
                    fillMode: Image.PreserveAspectFit
                    mipmap: true
                    smooth: true
                    color: canMoveDown ? qgcPalMoveDown.text : qgcPal.windowShade
                    source: "/InstrumentValueIcons/arrow-thin-down.svg"
                    enabled: canMoveDown

                    property bool canMoveDown: _videoSettings.streamConfigurations.canMoveDown(index)

                    QGCPalette {
                        id: qgcPalMoveDown
                        colorGroupEnabled: parent.enabled
                    }

                    QGCMouseArea {
                        fillItem: parent
                        onClicked: _videoSettings.streamConfigurations.moveStreamDown(index)
                    }
                }

                // Edit button
                QGCColoredImage {
                    height: ScreenTools.minTouchPixels
                    width: height
                    sourceSize.height: height
                    fillMode: Image.PreserveAspectFit
                    mipmap: true
                    smooth: true
                    color: qgcPalEdit.text
                    source: "/res/pencil.svg"

                    QGCPalette {
                        id: qgcPalEdit
                        colorGroupEnabled: parent.enabled
                    }

                    QGCMouseArea {
                        fillItem: parent
                        onClicked: {
                            streamEditDialogComponent.createObject(
                                mainWindow,
                                { originalStream: object }
                            ).open()
                        }
                    }
                }

                // Delete button
                QGCColoredImage {
                    height: ScreenTools.minTouchPixels
                    width: height
                    sourceSize.height: height
                    fillMode: Image.PreserveAspectFit
                    mipmap: true
                    smooth: true
                    color: qgcPalDelete.text
                    source: "/res/TrashDelete.svg"

                    QGCPalette {
                        id: qgcPalDelete
                        colorGroupEnabled: parent.enabled
                    }

                    QGCMouseArea {
                        fillItem: parent
                        onClicked: {
                            mainWindow.showMessageDialog(
                                qsTr("Delete Video Stream"),
                                qsTr("Are you sure you want to delete '%1'?").arg(object.name),
                                Dialog.Ok | Dialog.Cancel,
                                function() {
                                    _videoSettings.streamConfigurations.removeStream(index)
                                }
                            )
                        }
                    }
                }
            }
        }

        // Add new stream button
        LabelledButton {
            Layout.fillWidth: true
            label: qsTr("Add New Stream")
            buttonText: qsTr("Add")

            onClicked: {
                streamEditDialogComponent.createObject(
                    mainWindow,
                    { originalStream: null }
                ).open()
            }
        }

        // Empty state message
        QGCLabel {
            Layout.fillWidth: true
            text: qsTr("No video streams configured. Click 'Add' to create your first stream.")
            wrapMode: Text.WordWrap
            font.pointSize: ScreenTools.smallFontPointSize
            horizontalAlignment: Text.AlignHCenter
            visible: _videoSettings.streamConfigurations.count === 0
        }
    }

    // Edit/Add stream dialog component
    Component {
        id: streamEditDialogComponent

        QGCPopupDialog {
            title: originalStream ? qsTr("Edit Video Stream") : qsTr("Add New Video Stream")
            buttons: Dialog.Save | Dialog.Cancel
            acceptButtonEnabled: streamNameField.text.trim() !== "" &&
                               streamUrlField.text.trim() !== "" &&
                               !validationError

            property var originalStream: null
            property bool validationError: false
            property string errorMessage: ""

            onAccepted: {
                if (originalStream) {
                    // Edit existing stream
                    originalStream.name = streamNameField.text.trim()
                    originalStream.type = streamTypeCombo.currentValue
                    originalStream.url = streamUrlField.text.trim()
                    originalStream.enabled = streamEnabledCheckBox.checked
                } else {
                    // Add new stream
                    _videoSettings.streamConfigurations.addStream(
                        streamNameField.text.trim(),
                        streamTypeCombo.currentValue,
                        streamUrlField.text.trim(),
                        streamEnabledCheckBox.checked
                    )
                }
                _videoSettings.streamConfigurations.saveToSettings()
            }

            function validateUrl(type, url) {
                const trimmedUrl = url.trim()

                if (trimmedUrl === "") {
                    validationError = true
                    errorMessage = qsTr("URL cannot be empty")
                    return
                }

                // Type-specific validation
                if (type === _videoSettings.rtspVideoSource) {
                    if (!trimmedUrl.toLowerCase().startsWith("rtsp://")) {
                        validationError = true
                        errorMessage = qsTr("RTSP URL must start with rtsp://")
                        return
                    }
                } else if (type === _videoSettings.udp264VideoSource ||
                           type === _videoSettings.udp265VideoSource ||
                           type === _videoSettings.mpegtsVideoSource ||
                           type === _videoSettings.tcpVideoSource) {
                    // Validate IP:port format
                    const regex = /^[\d\.]+:\d+$/
                    if (!regex.test(trimmedUrl)) {
                        validationError = true
                        errorMessage = qsTr("URL must be in format: ip:port (e.g. 0.0.0.0:5600)")
                        return
                    }
                }

                validationError = false
                errorMessage = ""
            }

            ColumnLayout {
                spacing: ScreenTools.defaultFontPixelHeight

                // Section 1: Stream Information
                SettingsGroupLayout {
                    Layout.fillWidth: true
                    heading: qsTr("Stream Information")

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: ScreenTools.defaultFontPixelWidth

                        QGCLabel {
                            text: qsTr("Name")
                            Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 10
                        }
                        QGCTextField {
                            id: streamNameField
                            Layout.fillWidth: true
                            text: originalStream ? originalStream.name : ""
                            placeholderText: qsTr("Enter stream name")
                        }
                    }

                    QGCCheckBoxSlider {
                        id: streamEnabledCheckBox
                        Layout.fillWidth: true
                        text: qsTr("Enabled")
                        checked: originalStream ? originalStream.enabled : true
                    }
                }

                // Section 2: Stream Source
                SettingsGroupLayout {
                    Layout.fillWidth: true
                    heading: qsTr("Stream Source")

                    LabelledComboBox {
                        id: streamTypeCombo
                        Layout.fillWidth: true
                        label: qsTr("Type")
                        model: [
                            _videoSettings.rtspVideoSource,
                            _videoSettings.udp264VideoSource,
                            _videoSettings.udp265VideoSource,
                            _videoSettings.tcpVideoSource,
                            _videoSettings.mpegtsVideoSource
                        ]

                        property string currentValue: model[currentIndex]

                        Component.onCompleted: {
                            if (originalStream) {
                                const index = model.indexOf(originalStream.type)
                                currentIndex = index >= 0 ? index : 0
                            } else {
                                currentIndex = 0
                            }
                        }

                        onActivated: {
                            // Update URL placeholder and validate based on new type
                            validateUrl(currentValue, streamUrlField.text)
                        }
                    }
                }

                // Section 3: Connection
                SettingsGroupLayout {
                    Layout.fillWidth: true
                    heading: qsTr("Connection")

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: ScreenTools.defaultFontPixelHeight / 2

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: ScreenTools.defaultFontPixelWidth

                            QGCLabel {
                                text: urlLabel
                                Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 10

                                property string urlLabel: {
                                    const type = streamTypeCombo.currentValue
                                    if (type === _videoSettings.rtspVideoSource) {
                                        return qsTr("RTSP URL")
                                    } else if (type === _videoSettings.tcpVideoSource) {
                                        return qsTr("TCP URL")
                                    } else {
                                        return qsTr("UDP URL")
                                    }
                                }
                            }

                            QGCTextField {
                                id: streamUrlField
                                Layout.fillWidth: true
                                text: originalStream ? originalStream.url : ""
                                placeholderText: urlPlaceholder

                                property string urlPlaceholder: {
                                    const type = streamTypeCombo.currentValue
                                    if (type === _videoSettings.rtspVideoSource) {
                                        return "rtsp://192.168.1.100:554/live"
                                    } else if (type === _videoSettings.tcpVideoSource) {
                                        return "192.168.1.100:5600"
                                    } else {
                                        return "0.0.0.0:5600"
                                    }
                                }

                                onTextChanged: {
                                    validateUrl(streamTypeCombo.currentValue, text)
                                }
                            }
                        }

                        QGCLabel {
                            Layout.fillWidth: true
                            text: errorMessage
                            color: qgcPal.warningText
                            wrapMode: Text.WordWrap
                            visible: validationError
                        }
                    }
                }
            }
        }
    }

    SettingsGroupLayout {
        Layout.fillWidth:   true
        heading:            qsTr("Audio Settings")
        headingDescription: qsTr("Audio settings apply to streams that support audio (e.g., RTSP)")

        LabelledFactTextField {
            Layout.fillWidth:   true
            label:              qsTr("Volume")
            fact:               _videoSettings.audioVolume
            visible:            fact.visible
        }
    }

    SettingsGroupLayout {
        Layout.fillWidth:   true
        heading:            qsTr("Settings")
        visible:            !_videoAutoStreamConfig && _videoSettings.streamConfigurations.count > 0

        LabelledFactTextField {
            Layout.fillWidth:   true
            label:              qsTr("Aspect Ratio")
            fact:               _videoSettings.aspectRatio
            visible:            _videoSettings.aspectRatio.visible
        }

        FactCheckBoxSlider {
            Layout.fillWidth:   true
            text:               qsTr("Stop recording when disarmed")
            fact:               _videoSettings.disableWhenDisarmed
            visible:            fact.visible
        }

        FactCheckBoxSlider {
            Layout.fillWidth:   true
            text:               qsTr("Low Latency Mode")
            fact:               _videoSettings.lowLatencyMode
            visible:            fact.visible && _videoManager.gstreamerEnabled
        }

        LabelledFactComboBox {
            Layout.fillWidth:   true
            label:              fact.shortDescription
            fact:               _videoSettings.forceVideoDecoder
            visible:            fact.visible
            indexModel:         false
        }
    }

    SettingsGroupLayout {
        Layout.fillWidth: true
        heading:            qsTr("Local Video Storage")

        LabelledFactComboBox {
            Layout.fillWidth:   true
            label:              qsTr("Record File Format")
            fact:               _videoSettings.recordingFormat
            visible:            _videoSettings.recordingFormat.visible
        }

        FactCheckBoxSlider {
            Layout.fillWidth:   true
            text:               qsTr("Auto-Delete Saved Recordings")
            fact:               _videoSettings.enableStorageLimit
            visible:            fact.visible
        }

        LabelledFactTextField {
            Layout.fillWidth:   true
            label:              qsTr("Max Storage Usage")
            fact:               _videoSettings.maxVideoSize
            visible:            fact.visible
            enabled:            _videoSettings.enableStorageLimit.rawValue
        }
    }

}
