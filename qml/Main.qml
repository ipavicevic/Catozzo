import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Catozzo
import "components"

ApplicationWindow {
    id: root
    visible: true
    width: 1100
    height: 720
    title: {
        let base = "Catozzo " + Qt.application.version
        if (projectModel.filePath)
            return base + " — " + projectModel.filePath
        return base
    }

    ProjectModel {
        id: projectModel
        onReloadedFromDisk: statusBar.showMessage("Project reloaded from disk")
        onError: (msg) => statusBar.showMessage("Error: " + msg)
    }

    ClipScanner {
        id: scanner
        onScanProgress: (current, total) => statusBar.showMessage("Scanning " + current + "/" + total + "…")
        onError: (msg) => statusBar.showMessage("Error: " + msg)
    }

    FfmpegRunner {
        id: ffmpeg
        onFinished: (success, path) => statusBar.showMessage(success ? "Export complete: " + path : "Export failed")
        onError: (msg) => statusBar.showMessage("Error: " + msg)
        onLogLine: (line) => { if (line.trim()) console.log("[ffmpeg]", line.trim()) }
        onProgressChanged: exportProgress.value = ffmpeg.progress
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Toolbar {
            Layout.fillWidth: true
            projectModel: projectModel
            scanner: scanner
            ffmpeg: ffmpeg
        }

        ClipList {
            Layout.fillWidth: true
            Layout.fillHeight: true
            projectModel: projectModel
            scanner: scanner
        }

        ProgressBar {
            Layout.fillWidth: true
            id: exportProgress
            visible: ffmpeg.running
            from: 0; to: 100
        }

        Rectangle {
            Layout.fillWidth: true
            height: 24
            color: palette.window

            Label {
                id: statusBar
                anchors { left: parent.left; leftMargin: 8; verticalCenter: parent.verticalCenter }
                font.pixelSize: 12

                function showMessage(msg) { text = msg }
            }
        }
    }
}
