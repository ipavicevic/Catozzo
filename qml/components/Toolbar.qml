import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

ToolBar {
    required property var projectModel
    required property var scanner
    required property var ffmpeg

    RowLayout {
        anchors.fill: parent
        spacing: 4

        ToolButton {
            text: "New Project"
            onClicked: folderDialog.open()
        }

        ToolButton {
            text: "Open…"
            onClicked: openDialog.open()
        }

        ToolButton {
            text: "Save"
            enabled: projectModel.isDirty
            onClicked: projectModel.filePath ? projectModel.save() : saveDialog.open()
        }

        ToolSeparator {}

        ToolButton {
            text: ffmpeg.running ? "Cancel Export" : "Export"
            enabled: projectModel.data.clips !== undefined
            onClicked: ffmpeg.running ? ffmpeg.cancel() : ffmpeg.exportProject(projectModel.data)
        }

        Item { Layout.fillWidth: true }
    }

    FolderDialog {
        id: folderDialog
        title: "Select source folder"
        onAccepted: {
            let folder = selectedFolder.toString().replace(/^file:\/\/\//, "")
            projectModel.newProject(folder)
            let clips = scanner.scanFolder(folder)
            let data = Object.assign({}, projectModel.data)
            data["clips"] = clips
            projectModel.updateData(data)
            saveDialog.open()
        }
    }

    FileDialog {
        id: openDialog
        title: "Open project"
        nameFilters: ["Catozzo project (*.json)"]
        onAccepted: projectModel.load(selectedFile.toString().replace(/^file:\/\/\//, ""))
    }

    FileDialog {
        id: saveDialog
        title: "Save project as"
        fileMode: FileDialog.SaveFile
        nameFilters: ["Catozzo project (*.json)"]
        onAccepted: projectModel.saveAs(selectedFile.toString().replace(/^file:\/\/\//, ""))
    }
}
