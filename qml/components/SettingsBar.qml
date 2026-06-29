import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    required property var projectModel

    height: 40
    color: palette.window

    property var parsedData: JSON.parse(root.projectModel.dataJson || "{}")
    property bool hasProject: !!parsedData["transition"]

    function updateProject(patch) {
        let data = Object.assign({}, parsedData)
        for (let k in patch) data[k] = patch[k]
        root.projectModel.updateData(data)
    }

    RowLayout {
        anchors { fill: parent; leftMargin: 12; rightMargin: 12 }
        spacing: 12

        Label { text: "Transition:"; opacity: root.hasProject ? 1 : 0.4 }

        ComboBox {
            enabled: root.hasProject
            model: ["none", "crossfade", "fade to black"]
            currentIndex: {
                let t = root.parsedData["transition"]
                if (!t) return 0
                return ["none", "crossfade", "fade_black"].indexOf(t["type"] ?? "none")
            }
            onActivated: {
                let types = ["none", "crossfade", "fade_black"]
                let cur = root.parsedData["transition"] ?? { duration: 1.0 }
                root.updateProject({ transition: Object.assign({}, cur, { type: types[currentIndex] }) })
            }
        }

        Label { text: "Duration:"; opacity: root.hasProject ? 1 : 0.4 }

        SpinBox {
            enabled: root.hasProject && (root.parsedData["transition"]?.["type"] ?? "none") !== "none"
            from: 1; to: 50; stepSize: 5
            value: Math.round((root.parsedData["transition"]?.["duration"] ?? 1.0) * 10)
            textFromValue: (v) => (v / 10).toFixed(1) + "s"
            valueFromText: (t) => Math.round(parseFloat(t) * 10)
            onValueModified: {
                let cur = root.parsedData["transition"] ?? { type: "none" }
                root.updateProject({ transition: Object.assign({}, cur, { duration: value / 10 }) })
            }
        }

        ToolSeparator {}

        Label { text: "Fade in:"; opacity: root.hasProject ? 1 : 0.4 }
        SpinBox {
            enabled: root.hasProject
            from: 0; to: 50; stepSize: 5
            value: Math.round((root.parsedData["fade_in"] ?? 0) * 10)
            textFromValue: (v) => v === 0 ? "off" : (v / 10).toFixed(1) + "s"
            valueFromText: (t) => t === "off" ? 0 : Math.round(parseFloat(t) * 10)
            onValueModified: root.updateProject({ fade_in: value / 10 })
        }

        Label { text: "Fade out:"; opacity: root.hasProject ? 1 : 0.4 }
        SpinBox {
            enabled: root.hasProject
            from: 0; to: 50; stepSize: 5
            value: Math.round((root.parsedData["fade_out"] ?? 0) * 10)
            textFromValue: (v) => v === 0 ? "off" : (v / 10).toFixed(1) + "s"
            valueFromText: (t) => t === "off" ? 0 : Math.round(parseFloat(t) * 10)
            onValueModified: root.updateProject({ fade_out: value / 10 })
        }

        Item { Layout.fillWidth: true }
    }
}
