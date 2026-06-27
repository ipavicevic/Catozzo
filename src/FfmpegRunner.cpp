#include "FfmpegRunner.h"
#include <QJsonArray>
#include <QDir>
#include <QRegularExpression>
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
    qDebug() << "ffmpeg" << args;
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

    m_totalDuration = 0.0;
    for (const auto &c : clips)
        m_totalDuration += clipEffectiveDuration(c.toObject());

    // Write concat demuxer list file
    m_concatList.setFileTemplate(QDir::tempPath() + "/catozzo_concat_XXXXXX.txt");
    if (!m_concatList.open()) {
        emit error("Cannot create temporary concat list file.");
        return {};
    }
    for (const auto &c : clips) {
        QString path = QDir(sourceFolder).filePath(c.toObject()["file"].toString());
        path.replace("'", "'\\''");
        m_concatList.write(QString("file '%1'\n").arg(path).toUtf8());
    }
    m_concatList.flush();

    QStringList args;
    args << "-y"
         << "-f" << "concat"
         << "-safe" << "0"
         << "-i" << m_concatList.fileName()
         << "-c" << "copy"
         << m_outputPath;

    return args;
}
