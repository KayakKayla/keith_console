import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import KeithConsole 1.0

Window {
    id: root
    width: 960
    height: 600
    visible: true
    color: "#101010"
    title: qsTr("Keith Console")

    PlainTextSurface {
        id: surface
        anchors.fill: parent
        terminal: terminalBridge
        focus: true

        Component.onCompleted: forceActiveFocus()

        Keys.onPressed: event => {
            if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                terminalBridge.sendText("\r");
                event.accepted = true;
            } else if (event.key === Qt.Key_Backspace) {
                terminalBridge.sendText("\x7f");
                event.accepted = true;
            } else if (event.text.length > 0 && !event.text.match(/^[\x00-\x1F]$/)) {
                terminalBridge.sendText(event.text);
                event.accepted = true;
            }
        }

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.AllButtons
            onPressed: surface.forceActiveFocus()
        }
    }

    Binding {
        target: surface
        property: "fontFamily"
        value: terminalBridge.config["font.family"] !== undefined ? terminalBridge.config["font.family"] : "monospace"
    }

    Binding {
        target: surface
        property: "fontPointSize"
        value: terminalBridge.config["font.size"] !== undefined ? terminalBridge.config["font.size"] : 13
    }
}
