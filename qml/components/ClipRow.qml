import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    height: 56

    required property int index
    required property var clipData
    required property string sourceFolder
    required property var scanner

    signal clipModified(int idx, var updated)

    onClipDataChanged: enabledCheck.checked = clipData["enabled"] !== false

    color: index % 2 === 0 ? palette.base : palette.alternateBase

    RowLayout {
        anchors { fill: parent; leftMargin: 8; rightMargin: 8 }
        spacing: 0

        CheckBox {
            id: enabledCheck
            checked: root.clipData["enabled"] !== false
            onToggled: root.clipModified(root.index,
                Object.assign({}, root.clipData, { "enabled": checked }))
        }

        Image {
            width: 48
            height: 40
            fillMode: Image.PreserveAspectFit
            source: {
                if (!root.sourceFolder || !root.clipData["file"]) return ""
                let thm = root.scanner.findThumbnail(root.sourceFolder + "/" + root.clipData["file"])
                return thm ? "file:///" + thm : ""
            }
            Rectangle {
                anchors.fill: parent
                color: "#333"
                visible: parent.status !== Image.Ready
                Label { anchors.centerIn: parent; text: "▶"; color: "#888" }
            }
        }

        Label {
            text: root.clipData["file"] ?? ""
            Layout.preferredWidth: 220
            elide: Text.ElideMiddle
            opacity: (root.clipData["enabled"] ?? true) ? 1.0 : 0.4
        }

        Label {
            text: {
                let d = root.clipData["duration"] ?? 0
                return Math.floor(d / 60) + ":" + String(Math.floor(d % 60)).padStart(2, "0")
            }
            Layout.preferredWidth: 80
        }

        ComboBox {
            Layout.preferredWidth: 90
            model: ["default", "mute", "100%", "50%"]
            currentIndex: {
                let v = root.clipData["volume"]
                if (v === null || v === undefined) return 0
                if (v === 0.0) return 1
                if (v === 1.0) return 2
                if (v === 0.5) return 3
                return 0
            }
            onActivated: {
                let volumes = [null, 0.0, 1.0, 0.5]
                root.clipModified(root.index,
                    Object.assign({}, root.clipData, { "volume": volumes[currentIndex] }))
            }
        }

        ComboBox {
            Layout.preferredWidth: 120
            model: ["default", "none", "crossfade", "fade_black"]
            currentIndex: {
                let t = root.clipData["transition"]
                if (!t) return 0
                let idx = ["none", "crossfade", "fade_black"].indexOf(t["type"] ?? "")
                return idx >= 0 ? idx + 1 : 0
            }
            onActivated: {
                let types = [null, "none", "crossfade", "fade_black"]
                let type = types[currentIndex]
                let transition = type ? { "type": type, "duration": root.clipData["transition"]?.["duration"] ?? 1.0 } : null
                root.clipModified(root.index,
                    Object.assign({}, root.clipData, { "transition": transition }))
            }
        }

        SpinBox {
            Layout.preferredWidth: 90
            enabled: root.clipData["transition"] !== null && root.clipData["transition"] !== undefined
            from: 1; to: 50; stepSize: 5
            value: Math.round((root.clipData["transition"]?.["duration"] ?? 1.0) * 10)
            textFromValue: (v) => (v / 10).toFixed(1) + "s"
            valueFromText: (t) => Math.round(parseFloat(t) * 10)
            onValueModified: {
                let t = Object.assign({}, root.clipData["transition"], { "duration": value / 10 })
                root.clipModified(root.index,
                    Object.assign({}, root.clipData, { "transition": t }))
            }
        }
    }
}
