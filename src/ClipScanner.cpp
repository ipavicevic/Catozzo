#include "ClipScanner.h"
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

const QRegularExpression ClipScanner::s_goProPattern("^G[XH]\\d{6}$", QRegularExpression::CaseInsensitiveOption);

ClipScanner::ClipScanner(QObject *parent)
    : QObject(parent)
{}

QJsonArray ClipScanner::scanFolder(const QString &folderPath)
{
    QDir dir(folderPath);
    QStringList mp4s = dir.entryList({"*.MP4", "*.mp4"}, QDir::Files, QDir::Name);

    QJsonArray clips;
    int total = mp4s.size();

    for (int i = 0; i < total; ++i) {
        const QString &filename = mp4s[i];
        QString fullPath = dir.filePath(filename);

        emit scanProgress(i + 1, total);

        QJsonObject clip = probeClip(fullPath);
        clip["file"] = filename;
        clip["enabled"] = true;
        clip["volume"] = QJsonValue::Null;
        clip["segments"] = QJsonArray();
        clip["transition"] = QJsonValue::Null;

        clips.append(clip);
    }

    return clips;
}

QJsonObject ClipScanner::probeClip(const QString &filePath)
{
    QJsonObject result;

    QString output = runFfprobe(filePath);
    if (output.isEmpty()) return result;

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(output.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError) return result;

    QJsonObject root = doc.object();
    QJsonObject format = root["format"].toObject();
    QJsonObject tags = format["tags"].toObject();

    QString creationTime = tags["creation_time"].toString();
    if (creationTime.isEmpty())
        creationTime = tags["Creation Time"].toString();

    double duration = format["duration"].toString().toDouble();

    bool hasAudio = false;
    for (const QJsonValue &s : root["streams"].toArray()) {
        if (s.toObject()["codec_type"].toString() == "audio") {
            hasAudio = true;
            break;
        }
    }


    result["recorded"] = creationTime.isEmpty() ? QJsonValue::Null : QJsonValue(creationTime);
    result["duration"] = duration;
    result["has_audio"] = hasAudio;

    return result;
}

bool ClipScanner::isGoPro(const QString &filename)
{
    QString stem = QFileInfo(filename).baseName();
    return s_goProPattern.match(stem).hasMatch();
}

QString ClipScanner::findThumbnail(const QString &mp4Path)
{
    QFileInfo fi(mp4Path);
    QString thmPath = fi.dir().filePath(fi.baseName() + ".THM");
    return QFile::exists(thmPath) ? thmPath : QString();
}

QString ClipScanner::findLrv(const QString &mp4Path)
{
    QFileInfo fi(mp4Path);
    QString stem = fi.baseName();
    if (stem.size() < 2) return QString();

    QString lrvStem = "GL" + stem.mid(2);
    QString lrvPath = fi.dir().filePath(lrvStem + ".LRV");
    return QFile::exists(lrvPath) ? lrvPath : QString();
}

QString ClipScanner::runFfprobe(const QString &filePath) const
{
    QProcess process;
    process.start("ffprobe", {
        "-v", "quiet",
        "-print_format", "json",
        "-show_entries", "format_tags=creation_time:format=duration:stream=codec_type",
        filePath
    });

    if (!process.waitForFinished(10000)) {
        emit const_cast<ClipScanner*>(this)->error("ffprobe timed out for: " + filePath);
        return {};
    }

    return QString::fromUtf8(process.readAllStandardOutput());
}
