#include "ProjectModel.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDir>
#include <QDateTime>

ProjectModel::ProjectModel(QObject *parent)
    : QObject(parent)
{
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, &ProjectModel::onFileChanged);
}

bool ProjectModel::load(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        emit error("Cannot open file: " + path);
        return false;
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        emit error("Invalid JSON: " + parseError.errorString());
        return false;
    }

    if (!m_filePath.isEmpty())
        m_watcher.removePath(m_filePath);

    m_filePath = path;
    m_watcher.addPath(path);
    m_isDirty = false;

    setData(doc.object());
    emit filePathChanged();
    emit isDirtyChanged();
    return true;
}

bool ProjectModel::save()
{
    if (m_filePath.isEmpty()) return false;
    return saveAs(m_filePath);
}

bool ProjectModel::saveAs(const QString &path)
{
    QJsonObject updated = m_data;
    updated["modified"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    // On first save, default output filename to project name
    QJsonObject output = updated["output"].toObject();
    if (output["filename"].toString() == "combined.mp4") {
        QString name = QFileInfo(path).baseName();
        if (name.endsWith(".catozzo", Qt::CaseInsensitive))
            name.chop(8);
        output["filename"] = name + ".mp4";
        updated["output"] = output;
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        emit error("Cannot write file: " + path);
        return false;
    }

    m_writingFromApp = true;
    file.write(QJsonDocument(updated).toJson(QJsonDocument::Indented));
    file.close();

    if (m_filePath != path) {
        if (!m_filePath.isEmpty())
            m_watcher.removePath(m_filePath);
        m_filePath = path;
        m_watcher.addPath(path);
        emit filePathChanged();
    }

    m_data = updated;
    m_isDirty = false;
    emit isDirtyChanged();
    return true;
}

void ProjectModel::newProject(const QString &sourceFolder)
{
    if (!m_filePath.isEmpty())
        m_watcher.removePath(m_filePath);

    m_filePath = QString();
    m_isDirty = false;
    setData(buildNewProject(sourceFolder));
    emit filePathChanged();
    emit isDirtyChanged();
}

void ProjectModel::updateData(const QVariantMap &data)
{
    m_isDirty = true;
    setData(QJsonObject::fromVariantMap(data));
    emit isDirtyChanged();
}

void ProjectModel::onFileChanged(const QString &path)
{
    m_watcher.addPath(path);

    if (m_writingFromApp) {
        m_writingFromApp = false;
        return;
    }

    load(path);
    emit reloadedFromDisk();
}

void ProjectModel::setData(const QJsonObject &data)
{
    m_data = data;
    emit dataChanged();
}

QJsonObject ProjectModel::buildNewProject(const QString &sourceFolder) const
{
    QJsonObject project;
    project["schema_version"] = 1;
    project["created"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    project["modified"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    project["source_folder"] = sourceFolder;

    QJsonObject output;
    output["folder"] = sourceFolder;
    output["filename"] = "combined.mp4";
    project["output"] = output;

    QJsonObject transition;
    transition["type"] = "none";
    transition["duration"] = 1.0;
    project["transition"] = transition;
    project["fade_in"] = 0.0;
    project["fade_out"] = 0.0;

    project["default_volume"] = 0.0;
    project["clips"] = QJsonArray();

    return project;
}
