/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlyView
import QGroundControl.FlightMap

ColumnLayout {
    spacing: ScreenTools.defaultFontPixelHeight / 2

    TerrainProgress {
        Layout.fillWidth: true
    }

    // We use a Loader to load the photoVideoControlComponent only when we have an active vehicle and a camera manager.
    // This make it easier to implement PhotoVideoControl without having to check for the mavlink camera
    // to be null all over the place
    Loader {
        id:                 photoVideoControlLoader
        Layout.alignment:   Qt.AlignRight
        sourceComponent:    globals.activeVehicle && globals.activeVehicle.cameraManager ? photoVideoControlComponent : undefined

        property real rightEdgeCenterInset: visible ? parent.width - x : 0

        Component {
            id: photoVideoControlComponent

            PhotoVideoControl {
            }
        }
    }

    // Stream selector for multi-stream configuration
    // Only shown when multiple manual streams are configured
    FlyViewStreamSelector {
        Layout.alignment: Qt.AlignRight
        Layout.topMargin: ScreenTools.defaultFontPixelHeight / 2
    }

    // Voice streaming push-to-talk control
    // Only shown when voice streaming is enabled
    FlyViewVoiceStreamingControl {
        Layout.alignment: Qt.AlignRight
        Layout.topMargin: ScreenTools.defaultFontPixelHeight / 2
    }
}
