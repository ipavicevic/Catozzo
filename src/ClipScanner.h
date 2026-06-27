#pragma once

#include <QObject>
#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QRegularExpression>

class ClipScanner : public QObject
{
    Q_OBJECT

public:
    explicit ClipScanner(QObject *parent = nullptr);

    Q_INVOKABLE QJsonArray scanFolder(const QString &folderPath);
    Q_INVOKABLE QJsonObject probeClip(const QString &filePath);
    Q_INVOKABLE bool isGoPro(const QString &filename);
    Q_INVOKABLE QString findThumbnail(const QString &mp4Path);
    Q_INVOKABLE QString findLrv(const QString &mp4Path);

signals:
    void scanProgress(int current, int total);
    void error(const QString &message);

private:
    static const QRegularExpression s_goProPattern;
    QString runFfprobe(const QString &filePath) const;
};
