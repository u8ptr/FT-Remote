import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: window
    property var controller: app
    visible: true
    width: 1440
    height: 900
    minimumWidth: 1040
    minimumHeight: 680
    title: qsTr("FT Remote")
    color: "#0b1218"

    StackLayout {
        anchors.fill: parent
        currentIndex: controller.connected ? 1 : 0

        Login { app: controller }

        Rectangle {
            color: "#0b1218"
            ColumnLayout {
                anchors.fill: parent
                spacing: 0
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 42
                    color: "#121c25"
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 14
                        anchors.rightMargin: 14
                        Label { text: "FT Remote"; color: "#d7f9ff"; font.bold: true; font.pixelSize: 16 }
                        Label { text: controller.status; color: "#8ecae6"; Layout.fillWidth: true; leftPadding: 18 }
                        Label { text: controller.transmitting ? qsTr("TX") : qsTr("RX"); color: controller.transmitting ? "#ff6961" : "#6be7a0"; font.bold: true }
                    }
                }
                RowLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 0
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: 0
                        SpectrumView { Layout.fillWidth: true; Layout.fillHeight: true; Layout.preferredHeight: 1; values: controller.spectrum }
                        Rectangle { Layout.fillWidth: true; height: 1; color: "#294457" }
                        WaterfallView { Layout.fillWidth: true; Layout.fillHeight: true; Layout.preferredHeight: 2; row: controller.waterfallRow }
                    }
                    ControlsPanel { app: controller }
                }
            }
        }
    }
}
