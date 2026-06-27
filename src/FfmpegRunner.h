#pragma once

#include <QObject>
#include <QJsonObject>
#include <QVariantMap>
#include <QProcess>
#include <QTemporaryFile>

class FfmpegRunner : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
    Q_PROPERTY(int progress READ progress NOTIFY progressChanged)

public:
    explicit FfmpegRunner(QObject *parent = nullptr);
    ~FfmpegRunner();

    bool running() const { return m_running; }
    int progress() const { return m_progress; }

    Q_INVOKABLE void exportProject(const QVariantMap &project);
    Q_INVOKABLE void cancel();

signals:
    void runningChanged();
    void progressChanged();
    void finished(bool success, const QString &outputPath);
    void error(const QString &message);
    void logLine(const QString &line);

private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);
    void onStdErrReady();

private:
    QProcess m_process;
    QTemporaryFile m_concatList;
    bool m_running = false;
    int m_progress = 0;
    double m_totalDuration = 0.0;
    QString m_outputPath;

    void setRunning(bool running);
    void setProgress(int progress);
    QStringList buildArguments(const QJsonObject &project);
    double clipEffectiveDuration(const QJsonObject &clip) const;
};
