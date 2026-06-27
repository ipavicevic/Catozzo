import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ListView {
    id: root
    required property var projectModel
    required property var scanner

    clip: true
    model: projectModel.data["clips"] ?? []

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
        clipData: modelData
        clipIndex: index
        sourceFolder: root.projectModel.data["source_folder"] ?? ""
        scanner: root.scanner
        onClipChanged: (idx, updated) => {
            let clips = []
            let src = root.projectModel.data["clips"]
            for (let i = 0; i < src.length; i++)
                clips.push(i === idx ? updated : src[i])
            let data = Object.assign({}, root.projectModel.data)
            data["clips"] = clips
            root.projectModel.updateData(data)
        }
    }

    ScrollBar.vertical: ScrollBar {}
}
