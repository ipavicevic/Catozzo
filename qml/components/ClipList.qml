import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ListView {
    id: root
    required property var projectModel
    required property var scanner

    clip: true

    property var parsedData: JSON.parse(projectModel.dataJson || "{}")
    property string sourceFolder: parsedData["source_folder"] ?? ""
    property var clips: parsedData["clips"] ?? []
    model: clips.length

    header: Rectangle {
        width: root.width
        height: 32
        color: palette.mid

        RowLayout {
            anchors { fill: parent; leftMargin: 8; rightMargin: 8 }
            spacing: 0

            Item { width: 28 }
            Item { width: 56 }
            Label { text: "File"; Layout.preferredWidth: 220; font.bold: true }
            Label { text: "Duration"; Layout.preferredWidth: 80; font.bold: true }
            Label { text: "Volume"; Layout.preferredWidth: 90; font.bold: true }
            Label { text: "Transition"; Layout.preferredWidth: 120; font.bold: true }
            Label { text: "Trans. dur."; Layout.preferredWidth: 90; font.bold: true }
        }
    }

    delegate: ClipRow {
        width: root.width
        clipData: root.clips[index]
        sourceFolder: root.sourceFolder
        scanner: root.scanner
        onClipModified: (idx, updated) => {
            let newClips = root.clips.slice()
            newClips[idx] = updated
            let data = Object.assign({}, root.parsedData)
            data["clips"] = newClips
            root.projectModel.updateData(data)
        }
    }

    ScrollBar.vertical: ScrollBar {}
}
