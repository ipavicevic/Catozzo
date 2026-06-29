#include "FfmpegRunner.h"
#include <QJsonArray>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStorageInfo>
#include <QDebug>

FfmpegRunner::FfmpegRunner(QObject *parent)
    : QObject(parent)
{
    connect(&m_process, &QProcess::readyReadStandardError, this, &FfmpegRunner::onStdErrReady);
    connect(&m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &FfmpegRunner::onProcessFinished);
}

FfmpegRunner::~FfmpegRunner()
{
    if (m_process.state() != QProcess::NotRunning) {
        m_process.kill();
        m_process.waitForFinished(3000);
    }
}

void FfmpegRunner::exportProject(const QVariantMap &project)
{
    if (m_running) return;

    QStringList args = buildArguments(QJsonObject::fromVariantMap(project));
    if (args.isEmpty()) return;

    setRunning(true);
    setProgress(0);
    qDebug() << "ffmpeg" << args.join(" ");
    m_process.start("ffmpeg", args);
}

void FfmpegRunner::cancel()
{
    if (m_running) m_process.kill();
}

void FfmpegRunner::onProcessFinished(int exitCode, QProcess::ExitStatus)
{
    setRunning(false);
    emit finished(exitCode == 0, m_outputPath);
}

void FfmpegRunner::onStdErrReady()
{
    QString line = QString::fromUtf8(m_process.readAllStandardError());
    emit logLine(line);

    static QRegularExpression timeRe("time=(\\d+):(\\d+):(\\d+\\.\\d+)");
    auto match = timeRe.match(line);
    if (match.hasMatch() && m_totalDuration > 0) {
        double elapsed = match.captured(1).toDouble() * 3600
                       + match.captured(2).toDouble() * 60
                       + match.captured(3).toDouble();
        setProgress(static_cast<int>(100.0 * elapsed / m_totalDuration));
    }
}

void FfmpegRunner::setRunning(bool running)
{
    if (m_running == running) return;
    m_running = running;
    emit runningChanged();
}

void FfmpegRunner::setProgress(int progress)
{
    if (m_progress == progress) return;
    m_progress = progress;
    emit progressChanged();
}

double FfmpegRunner::clipEffectiveDuration(const QJsonObject &clip) const
{
    QJsonArray segments = clip["segments"].toArray();
    if (segments.isEmpty())
        return clip["duration"].toDouble();

    double total = 0.0;
    for (const auto &seg : segments)
        total += seg.toObject()["end"].toDouble() - seg.toObject()["start"].toDouble();
    return total;
}


QStringList FfmpegRunner::buildArguments(const QJsonObject &project)
{
    QString sourceFolder = project["source_folder"].toString();
    QJsonObject output = project["output"].toObject();
    m_outputPath = QDir(output["folder"].toString()).filePath(output["filename"].toString());

    QJsonObject transition = project["transition"].toObject();
    QString trType = transition["type"].toString();
    double trDur   = transition["duration"].toDouble(1.0);
    double fadeIn  = project["fade_in"].toDouble(0.0);
    double fadeOut = project["fade_out"].toDouble(0.0);

    bool hasTransition = (trType != "none" && !trType.isEmpty());
    bool hasFade       = (fadeIn > 0.0 || fadeOut > 0.0);

    QJsonArray allClips = project["clips"].toArray();
    QJsonArray clips;
    for (const auto &c : allClips) {
        if (c.toObject()["enabled"].toBool())
            clips.append(c);
    }

    if (clips.isEmpty()) {
        emit error("No enabled clips to export.");
        return {};
    }

    int n = clips.size();

    m_totalDuration = 0.0;
    QVector<double> durations(n);
    for (int i = 0; i < n; ++i) {
        durations[i] = clipEffectiveDuration(clips[i].toObject());
        m_totalDuration += durations[i];
    }

    bool allHaveAudio = true;
    for (const auto &c : clips)
        if (!c.toObject()["has_audio"].toBool())
            allHaveAudio = false;

    // Disk space check
    qint64 estimatedBytes = 0;
    if (!hasTransition && !hasFade) {
        // Stream copy: output ≈ sum of input sizes
        for (const auto &c : clips) {
            QString path = QDir(sourceFolder).filePath(c.toObject()["file"].toString());
            estimatedBytes += QFileInfo(path).size();
        }
    } else {
        // Re-encode: estimate at ~8 Mbps (CRF 28 ultrafast, typical for 1080p/4K)
        estimatedBytes = static_cast<qint64>(m_totalDuration * 8000000 / 8);
    }
    QStorageInfo storage(QFileInfo(m_outputPath).absoluteDir().absolutePath());
    qint64 available = storage.bytesAvailable();
    if (estimatedBytes > available) {
        emit error(QString("Not enough disk space. Estimated output: %1 GB, available: %2 GB.")
            .arg(estimatedBytes / 1e9, 0, 'f', 1)
            .arg(available / 1e9, 0, 'f', 1));
        return {};
    }

    QStringList args;
    args << "-y";

    if (!hasTransition && !hasFade) {
        // Fast path: stream copy via concat demuxer
        m_concatList = std::make_unique<QTemporaryFile>(
            QDir::tempPath() + "/catozzo_concat_XXXXXX.txt");
        if (!m_concatList->open()) {
            emit error("Cannot create temporary concat list file.");
            return {};
        }
        for (const auto &c : clips) {
            QString path = QDir(sourceFolder).filePath(c.toObject()["file"].toString());
            path.replace("'", "'\\''");
            m_concatList->write(QString("file '%1'\n").arg(path).toUtf8());
        }
        m_concatList->flush();

        args << "-f" << "concat" << "-safe" << "0"
             << "-i" << m_concatList->fileName()
             << "-c" << "copy"
             << m_outputPath;
        return args;
    }

    // Re-encode path: transitions and/or fades
    for (int i = 0; i < n; ++i)
        args << "-i" << QDir(sourceFolder).filePath(clips[i].toObject()["file"].toString());

    // Use a counter for intermediate labels so [outv]/[outa] are only assigned once at the end
    int labelIdx = 0;
    auto nextLabel = [&](const QString &prefix) {
        return QString("[%1%2]").arg(prefix).arg(labelIdx++);
    };

    QString filterComplex;
    QString prevV = "[0:v]";
    QString prevA = allHaveAudio ? "[0:a]" : "";
    double cumDuration = durations[0];

    // Fade in
    if (fadeIn > 0.0) {
        QString outV = nextLabel("v");
        filterComplex += QString("[0:v]fade=t=in:st=0:d=%1%2;").arg(fadeIn).arg(outV);
        prevV = outV;
        if (allHaveAudio) {
            QString outA = nextLabel("a");
            filterComplex += QString("[0:a]afade=t=in:st=0:d=%1%2;").arg(fadeIn).arg(outA);
            prevA = outA;
        }
    }

    // Chain clips
    QString xfadeType = (trType == "crossfade") ? "dissolve" : "fade";
    for (int i = 1; i < n; ++i) {
        QString curV = QString("[%1:v]").arg(i);
        QString curA = allHaveAudio ? QString("[%1:a]").arg(i) : "";
        QString outV = nextLabel("v");
        QString outA = allHaveAudio ? nextLabel("a") : "";

        if (hasTransition) {
            double offset = cumDuration - trDur;
            filterComplex += QString("%1%2xfade=transition=%3:duration=%4:offset=%5%6;")
                .arg(prevV, curV, xfadeType).arg(trDur).arg(offset).arg(outV);
            if (allHaveAudio)
                filterComplex += QString("%1%2acrossfade=d=%3%4;")
                    .arg(prevA, curA).arg(trDur).arg(outA);
            cumDuration += durations[i] - trDur;
        } else {
            filterComplex += QString("%1%2concat=n=2:v=1:a=0%3;").arg(prevV, curV, outV);
            if (allHaveAudio)
                filterComplex += QString("%1%2concat=n=2:v=0:a=1%3;").arg(prevA, curA, outA);
            cumDuration += durations[i];
        }

        prevV = outV;
        prevA = outA;
    }

    // Fade out — prevV/prevA are always intermediates here, safe to consume
    if (fadeOut > 0.0) {
        double startTime = cumDuration - fadeOut;
        filterComplex += QString("%1fade=t=out:st=%2:d=%3[outv];")
            .arg(prevV).arg(startTime).arg(fadeOut);
        if (allHaveAudio)
            filterComplex += QString("%1afade=t=out:st=%2:d=%3[outa];")
                .arg(prevA).arg(startTime).arg(fadeOut);
    } else {
        filterComplex += QString("%1copy[outv];").arg(prevV);
        if (allHaveAudio)
            filterComplex += QString("%1acopy[outa];").arg(prevA);
    }

    if (filterComplex.endsWith(';'))
        filterComplex.chop(1);

    args << "-filter_complex" << filterComplex;
    args << "-map" << "[outv]";
    if (allHaveAudio) args << "-map" << "[outa]";
    args << "-c:v" << "libx264" << "-crf" << "28" << "-preset" << "ultrafast"
         << "-profile:v" << "high" << "-level:v" << "4.2" << "-pix_fmt" << "yuv420p";
    if (allHaveAudio) args << "-c:a" << "aac" << "-b:a" << "192k";
    args << m_outputPath;

    return args;
}
