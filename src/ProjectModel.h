#pragma once

#include <QObject>
#include <QJsonObject>
#include <QFileSystemWatcher>
#include <QString>

class ProjectModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString filePath READ filePath NOTIFY filePathChanged)
    Q_PROPERTY(QJsonObject data READ data NOTIFY dataChanged)
    Q_PROPERTY(bool isDirty READ isDirty NOTIFY isDirtyChanged)

public:
    explicit ProjectModel(QObject *parent = nullptr);

    QString filePath() const { return m_filePath; }
    QJsonObject data() const { return m_data; }
    bool isDirty() const { return m_isDirty; }

    Q_INVOKABLE bool load(const QString &path);
    Q_INVOKABLE bool save();
    Q_INVOKABLE bool saveAs(const QString &path);
    Q_INVOKABLE void newProject(const QString &sourceFolder);
    Q_INVOKABLE void updateData(const QJsonObject &data);

signals:
    void filePathChanged();
    void dataChanged();
    void isDirtyChanged();
    void reloadedFromDisk();
    void error(const QString &message);

private slots:
    void onFileChanged(const QString &path);

private:
    QString m_filePath;
    QJsonObject m_data;
    bool m_isDirty = false;
    bool m_writingFromApp = false;
    QFileSystemWatcher m_watcher;

    void setData(const QJsonObject &data);
    QJsonObject buildNewProject(const QString &sourceFolder) const;
};
