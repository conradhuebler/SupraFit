/*
 * SupraFit Project Manager - Centralized Project Management Implementation
 * Copyright (C) 2025 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
 *
 * Implementation of centralized project management for CLI/GUI integration
 * Claude Generated - Educational-First Design Implementation
 */

#include "projectmanager.h"

#include "src/core/loadtiming.h"

#include "src/core/filehandler.h"
#include "src/core/jsonhandler.h"
#include "src/core/models/dataclass.h"
#include "src/core/models/models.h"  // Claude Generated - Required for CreateModel factory function
#include "src/core/toolset.h"

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonDocument>
#include <QtCore/QLoggingCategory>
#include <QtCore/QMutexLocker>
#include <QtCore/QStandardPaths>

#include "src/core/models/meta_model.h"
#include "src/version.h"

#include <algorithm>
#include <cmath>
#include <limits>

#ifdef DEBUG_ON
#include "fmt/core.h"
#endif

Q_LOGGING_CATEGORY(projectManager, "suprafit.core.projectmanager")

namespace SupraFit {

// Static member initialization
QMutex ProjectManager::s_instanceMutex;
ProjectManager* ProjectManager::s_instance = nullptr;

ProjectManager& ProjectManager::instance()
{
    #ifdef DEBUG_ON
    qCDebug(projectManager) << "ProjectManager::instance() called";
    #endif
    QMutexLocker locker(&s_instanceMutex);
    if (!s_instance) {
        #ifdef DEBUG_ON
        qCDebug(projectManager) << "Creating new ProjectManager singleton instance";
        #endif
        s_instance = new ProjectManager();
        qCDebug(projectManager) << "ProjectManager singleton instance created";
    }
    return *s_instance;
}

ProjectManager::ProjectManager(QObject* parent)
    : QObject(parent)
    , m_currentProjectId(QString())
{
    qCDebug(projectManager) << "ProjectManager constructor called";
}

ProjectManager::~ProjectManager()
{
    QMutexLocker locker(&m_projectsMutex);
    m_projects.clear();
    m_projectHash.clear();
    qCDebug(projectManager) << "ProjectManager destructor - all projects cleared";
}

// === Core Project Management ===

bool ProjectManager::loadProject(const QString& filePath)
{
    LoadTiming::Scope loadTiming("pm.loadProject");
    QMutexLocker locker(&m_projectsMutex);

    m_report = LoadReport();
    m_report.file = filePath;
    m_reportOpen = true;
    auto fail = [&](const QString& scope, const QString& reason) {
        addIssue(LoadIssue::Error, scope, reason);
        emit errorOccurred("loadProject", QString("%1: %2").arg(filePath, reason));
        m_reportOpen = false;
        m_report.loaded = false;
        return false;
    };

    if (filePath.isEmpty())
        return fail("file", tr("No file name was given."));

    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists())
        return fail("file", tr("The file does not exist."));

    try {
        // A project file (.suprafit/.json) is read once, by JsonHandler; FileHandler would read and parse it a second time only to say
        // that it is one (its type of a project file is the suffix alone). Other formats go through FileHandler.
        QJsonObject projectData;
        const QString suffix = fileInfo.suffix().toLower();
        if (suffix == QLatin1String("suprafit") || suffix == QLatin1String("json")) {
            // Load SupraFit project file: the reason of a failure is part of the report
            QString reason;
            if (!JsonHandler::ReadFile(filePath, projectData, &reason, &m_report.format))
                return fail("file", reason);
        } else {
            FileHandler handler(filePath);
            handler.LoadFile();
            projectData = handler.getJsonData();
        }

        // Validate and create project
        const QString problem = projectJsonProblem(projectData);
        if (!problem.isEmpty())
            return fail("file", problem);

        QString projectId;
        QStringList projectKeys; // a file of several projects: "project_0", "project_1", ... each a project of its own
        if (!projectData.contains("data"))
            for (const QString& key : projectData.keys())
                if (key.startsWith(QLatin1String("project_")) && projectData[key].isObject())
                    projectKeys << key;
        if (!projectKeys.isEmpty()) {
            std::sort(projectKeys.begin(), projectKeys.end(), [](const QString& a, const QString& b) { return a.mid(8).toInt() < b.mid(8).toInt(); });
            m_report.projectsInFile = int(projectKeys.size());
            const QString base = fileInfo.completeBaseName();
            int number = 0;
            QList<QPair<QString, QJsonObject>> metaBlocks;
            for (const QString& key : projectKeys) {
                ++number;
                const QJsonObject single = projectData[key].toObject();
                m_scope = key;
                const QJsonObject block = single["data"].toObject();
                if (block.contains("model") && block["model"].toInt() == int(SupraFit::MetaModel)) {
                    // a meta model compares the models of other projects of the file: it is rebuilt when they are all there (below)
                    metaBlocks << qMakePair(key, block);
                } else if (!single.contains("data")) {
                    addIssue(LoadIssue::Warning, "file", tr("Left out: the entry holds no data."));
                } else {
                    const QString id = loadProjectFromJson(single, QStringLiteral("%1-%2").arg(base).arg(number));
                    if (id.isEmpty())
                        addIssue(LoadIssue::Warning, "file", tr("Left out: the project could not be created."));
                    else {
                        ++m_report.projectsLoaded;
                        if (projectId.isEmpty())
                            projectId = id;
                    }
                }
                m_scope.clear();
            }
            for (const auto& meta : std::as_const(metaBlocks)) {
                m_scope = meta.first;
                const QString id = restoreMetaModel(meta.second);
                if (!id.isEmpty()) {
                    ++m_report.projectsLoaded;
                    if (projectId.isEmpty())
                        projectId = id;
                }
                m_scope.clear();
            }
            m_report.projectId = projectId;
            if (projectId.isEmpty())
                return fail("file", tr("None of the %1 projects of the file could be read.").arg(projectKeys.size()));
        } else {
            projectId = loadProjectFromJson(projectData, filePath);
            if (projectId.isEmpty())
                return fail("data", m_report.issues.isEmpty() ? tr("The project could not be created from the file.") : tr("The project could not be created from the file (see above)."));
        }

        // Set as current project if none is active
        if (m_currentProjectId.isEmpty()) {
            m_currentProjectId = projectId;
        }

        updateProjectHash();

        emit projectLoaded(projectId, filePath);
        m_reportOpen = false;
        m_report.loaded = true;
        return true;

    } catch (const std::exception& e) {
        return fail("file", tr("Exception while reading the file: %1").arg(e.what()));
    }
}

bool ProjectManager::saveProject(const QString& filePath, const QString& projectId)
{
    QMutexLocker locker(&m_projectsMutex);

    QString targetProjectId = projectId.isEmpty() ? m_currentProjectId : projectId;

    if (targetProjectId.isEmpty()) {
        emit errorOccurred("saveProject", "No project specified and no current project set");
        return false;
    }

    auto projectIt = m_projectHash.find(targetProjectId);
    if (projectIt == m_projectHash.end()) {
        emit errorOccurred("saveProject", QString("Project not found: %1").arg(targetProjectId));
        return false;
    }

    QSharedPointer<DataClass> project = projectIt.value().toStrongRef();
    if (!project) {
        emit errorOccurred("saveProject", QString("Project reference is null: %1").arg(targetProjectId));
        return false;
    }

#ifdef DEBUG_ON
    SFDebugPrint("🔍 DEBUG ProjectManager::saveProject: Saving project {} to {}\n",
        targetProjectId.toStdString(), filePath.toStdString());
#endif

    try {
        QJsonObject projectData = saveProjectAsJson(project);

        // Determine output format based on file extension
        QString extension = QFileInfo(filePath).suffix().toLower();
        bool success = false;

        if (extension == "json") {
            // Save as human-readable JSON
            QJsonDocument doc(projectData);
            QFile file(filePath);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(doc.toJson());
                file.close();
                success = true;
            }
        } else {
            // Save as compressed SupraFit format
            success = JsonHandler::WriteJsonFile(projectData, filePath);
        }

        if (success) {
            m_unsavedAttachments.remove(targetProjectId);
            emit projectSaved(targetProjectId, filePath);

#ifdef DEBUG_ON
            SFDebugPrint("✅ DEBUG ProjectManager::saveProject: Successfully saved project {} to {}\n",
                targetProjectId.toStdString(), filePath.toStdString());
#endif
            return true;
        } else {
            emit errorOccurred("saveProject", QString("Failed to write project file: %1").arg(filePath));
            return false;
        }

    } catch (const std::exception& e) {
        emit errorOccurred("saveProject", QString("Exception during project saving: %1").arg(e.what()));
        return false;
    }
}

bool ProjectManager::saveAllProjects(const QString& filePath)
{
    QMutexLocker locker(&m_projectsMutex);

    if (m_projects.isEmpty()) {
        emit errorOccurred("saveAllProjects", "No projects to save");
        return false;
    }

#ifdef DEBUG_ON
    SFDebugPrint("🔍 DEBUG ProjectManager::saveAllProjects: Saving {} projects to {}\n",
        m_projects.size(), filePath.toStdString());
#endif

    try {
        QJsonArray projectsArray;

        for (const auto& project : m_projects) {
            if (project) {
                QJsonObject projectData = saveProjectAsJson(project);
                projectsArray.append(projectData);
            }
        }

        QJsonObject topLevel;
        topLevel["projects"] = projectsArray;
        topLevel["version"] = "SupraFit-ProjectManager-1.0";
        topLevel["projectCount"] = projectsArray.size();

        // Save as JSON document
        QJsonDocument doc(topLevel);
        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(doc.toJson());
            file.close();

#ifdef DEBUG_ON
            SFDebugPrint("✅ DEBUG ProjectManager::saveAllProjects: Successfully saved {} projects\n",
                projectsArray.size());
#endif
            return true;
        } else {
            emit errorOccurred("saveAllProjects", QString("Failed to open file for writing: %1").arg(filePath));
            return false;
        }

    } catch (const std::exception& e) {
        emit errorOccurred("saveAllProjects", QString("Exception during batch save: %1").arg(e.what()));
        return false;
    }
}

// === Project State Management ===

QWeakPointer<DataClass> ProjectManager::getCurrentProject() const
{
    QMutexLocker locker(&m_projectsMutex);

    if (m_currentProjectId.isEmpty()) {
        return QWeakPointer<DataClass>();
    }

    auto it = m_projectHash.find(m_currentProjectId);
    if (it != m_projectHash.end()) {
        return it.value();
    }

    return QWeakPointer<DataClass>();
}

QSharedPointer<DataClass> ProjectManager::getProjectData(const QString& projectId) const
{
    QMutexLocker locker(&m_projectsMutex);
    
    if (m_projectHash.contains(projectId)) {
        auto weakPtr = m_projectHash.value(projectId);
        return weakPtr.toStrongRef();
    }
    
    return QSharedPointer<DataClass>();
}

QWeakPointer<DataClass> ProjectManager::getWeakProjectData(const QString& projectId) const
{
    QMutexLocker locker(&m_projectsMutex);
    
    if (m_projectHash.contains(projectId)) {
        return m_projectHash.value(projectId);
    }
    
    return QWeakPointer<DataClass>();
}

QStringList ProjectManager::getLoadedProjectIds() const
{
    QMutexLocker locker(&m_projectsMutex);
    return m_projectHash.keys();
}

QJsonObject ProjectManager::getProjectJson(const QString& projectId) const
{
    QMutexLocker locker(&m_projectsMutex);
    
    if (m_projectHash.contains(projectId)) {
        auto weakPtr = m_projectHash.value(projectId);
        QSharedPointer<DataClass> project = weakPtr.toStrongRef();
        if (project) {
            return project->ExportData();
        }
    }
    
    return QJsonObject();
}

bool ProjectManager::setCurrentProject(const QString& projectId)
{
    QMutexLocker locker(&m_projectsMutex);

    if (projectId.isEmpty()) {
        QString oldProjectId = m_currentProjectId;
        m_currentProjectId.clear();
        emit currentProjectChanged(m_currentProjectId);
        return true;
    }

    if (m_projectHash.contains(projectId)) {
        QString oldProjectId = m_currentProjectId;
        m_currentProjectId = projectId;

        if (oldProjectId != m_currentProjectId) {
            emit currentProjectChanged(m_currentProjectId);
        }

        return true;
    }

    emit errorOccurred("setCurrentProject", QString("Project not found: %1").arg(projectId));
    return false;
}

QWeakPointer<DataClass> ProjectManager::getProject(const QString& projectId) const
{
    QMutexLocker locker(&m_projectsMutex);

    auto it = m_projectHash.find(projectId);
    if (it != m_projectHash.end()) {
        return it.value();
    }

    return QWeakPointer<DataClass>();
}

bool ProjectManager::addModelToProject(QSharedPointer<AbstractModel> model, const QString& projectId)
{
    QMutexLocker locker(&m_projectsMutex);

    if (!model) {
        emit errorOccurred("addModelToProject", "Model pointer is null");
        return false;
    }

    QString targetProjectId = projectId.isEmpty() ? m_currentProjectId : projectId;

    if (targetProjectId.isEmpty()) {
        emit errorOccurred("addModelToProject", "No project specified and no current project set");
        return false;
    }

    auto projectIt = m_projectHash.find(targetProjectId);
    if (projectIt == m_projectHash.end()) {
        emit errorOccurred("addModelToProject", QString("Project not found: %1").arg(targetProjectId));
        return false;
    }

    QSharedPointer<DataClass> project = projectIt.value().toStrongRef();
    if (!project) {
        emit errorOccurred("addModelToProject", QString("Project reference is null: %1").arg(targetProjectId));
        return false;
    }

    try {
        // Models are managed separately from DataClass in SupraFit architecture
        // DataClass contains data, models reference that data but are stored separately
        // This is correct design - just emit the signal to notify GUI components
#ifdef DEBUG_ON
        qDebug() << "ProjectManager: Model" << model->ModelUUID() << "conceptually added to project" << targetProjectId;
#endif
        emit modelAdded(targetProjectId, model->ModelUUID());

#ifdef DEBUG_ON
        SFDebugPrint("✅ DEBUG ProjectManager::addModelToProject: Added model {} to project {}\n",
            model->ModelUUID().toStdString(), targetProjectId.toStdString());
#endif

        return true;

    } catch (const std::exception& e) {
        emit errorOccurred("addModelToProject", QString("Exception adding model: %1").arg(e.what()));
        return false;
    }
}

QString ProjectManager::getCurrentProjectId() const
{
    QMutexLocker locker(&m_projectsMutex);
    return m_currentProjectId;
}

QJsonValue ProjectManager::projectAttachment(const QString& projectId, const QString& key) const
{
    QMutexLocker locker(&m_projectsMutex);
    const auto it = m_attachments.constFind(projectId);
    if (it == m_attachments.constEnd() || !it->contains(key))
        return QJsonValue(QJsonValue::Undefined);
    return it->value(key);
}

void ProjectManager::setProjectAttachment(const QString& projectId, const QString& key, const QJsonValue& value)
{
    {
        QMutexLocker locker(&m_projectsMutex);
        QJsonObject& attachments = m_attachments[projectId];
        if (value.isUndefined() || value.isNull()) {
            if (!attachments.contains(key))
                return;
            attachments.remove(key);
        } else {
            if (attachments.value(key) == value)
                return;
            attachments.insert(key, value);
        }
        m_unsavedAttachments.insert(projectId);
    }
    emit projectAttachmentChanged(projectId, key);
}

bool ProjectManager::hasUnsavedAttachments(const QString& projectId) const
{
    QMutexLocker locker(&m_projectsMutex);
    return m_unsavedAttachments.contains(projectId);
}

QStringList ProjectManager::projectsWithUnsavedAttachments() const
{
    QMutexLocker locker(&m_projectsMutex);
    QStringList ids;
    for (const QString& id : m_unsavedAttachments)
        if (m_projectHash.contains(id))
            ids << id;
    return ids;
}

QJsonObject ProjectManager::getProjectFileJson(const QString& projectId) const
{
    QMutexLocker locker(&m_projectsMutex);
    const auto it = m_projectHash.constFind(projectId);
    if (it == m_projectHash.constEnd())
        return QJsonObject();
    return saveProjectAsJson(it.value().toStrongRef());
}

bool ProjectManager::removeProject(const QString& projectId)
{
    QMutexLocker locker(&m_projectsMutex);

    auto hashIt = m_projectHash.find(projectId);
    if (hashIt == m_projectHash.end()) {
        emit errorOccurred("removeProject", QString("Project not found: %1").arg(projectId));
        return false;
    }

    // Remove from project vector
    for (int i = 0; i < m_projects.size(); ++i) {
        if (m_projects[i] && m_projects[i]->UUID() == projectId) {
            m_projects.removeAt(i);
            break;
        }
    }

    // Remove from hash
    m_projectHash.erase(hashIt);

    // Update current project if it was removed
    if (m_currentProjectId == projectId) {
        if (!m_projects.isEmpty()) {
            m_currentProjectId = m_projects.first()->UUID();
            emit currentProjectChanged(m_currentProjectId);
        } else {
            m_currentProjectId.clear();
            emit currentProjectChanged(QString());
        }
    }

    m_attachments.remove(projectId);
    m_unsavedAttachments.remove(projectId);
    emit projectRemoved(projectId);

#ifdef DEBUG_ON
    SFDebugPrint("✅ DEBUG ProjectManager::removeProject: Removed project {}\n", projectId.toStdString());
#endif

    return true;
}

// === CLI Compatibility Interface ===

QJsonObject ProjectManager::getProjectAsJson(const QString& projectId) const
{
    QMutexLocker locker(&m_projectsMutex);

    QString targetProjectId = projectId.isEmpty() ? m_currentProjectId : projectId;

    if (targetProjectId.isEmpty()) {
        return QJsonObject();
    }

    auto it = m_projectHash.find(targetProjectId);
    if (it != m_projectHash.end()) {
        QSharedPointer<DataClass> project = it.value().toStrongRef();
        if (project) {
            return saveProjectAsJson(project);
        }
    }

    return QJsonObject();
}

QString ProjectManager::createProjectFromJson(const QJsonObject& projectJson, const QString& projectTitle)
{
    QMutexLocker locker(&m_projectsMutex);

    if (!validateProjectJson(projectJson)) {
        emit errorOccurred("createProjectFromJson", "Invalid project JSON structure");
        return QString();
    }

    // Check for Multi-Project format (Claude Generated - CLI Multi-Project Support)
    if (projectJson.contains("format_version") && projectJson["format_version"].toString() == "2.0") {
        // Multi-Project format: create multiple projects and return the first one's ID
        QString firstProjectId;
        QStringList createdProjects;

        for (auto it = projectJson.begin(); it != projectJson.end(); ++it) {
            if (it.key().startsWith("project_")) {
                QJsonObject singleProjectJson = it.value().toObject();
                QString projectName = QString("%1 %2").arg(projectTitle.isEmpty() ? "Generated Project" : projectTitle).arg(it.key());

                QString projectId = loadProjectFromJson(singleProjectJson, projectName);
                if (!projectId.isEmpty()) {
                    createdProjects.append(projectId);
                    if (firstProjectId.isEmpty()) {
                        firstProjectId = projectId;
                    }
                    emit projectAdded(projectId, projectName);
#ifdef DEBUG_ON
                    SFDebugPrint("✅ DEBUG ProjectManager::createProjectFromJson: Created multi-project {} with title '{}'\n",
                        projectId.toStdString(), projectName.toStdString());
#endif
                }
            }
        }

        if (!firstProjectId.isEmpty()) {
            updateProjectHash();
            // Set as current project if none is active
            if (m_currentProjectId.isEmpty()) {
                m_currentProjectId = firstProjectId;
            }
        }

        return firstProjectId;
    }

    // Standard single-project format
    QString projectId = loadProjectFromJson(projectJson, projectTitle);
    if (!projectId.isEmpty()) {
        updateProjectHash();

        // Set as current project if none is active
        if (m_currentProjectId.isEmpty()) {
            m_currentProjectId = projectId;
        }

        emit projectAdded(projectId, projectTitle.isEmpty() ? "Generated Project" : projectTitle);

#ifdef DEBUG_ON
        SFDebugPrint("✅ DEBUG ProjectManager::createProjectFromJson: Created project {} with title '{}'\n",
            projectId.toStdString(), projectTitle.toStdString());
#endif
    }

    return projectId;
}

QVector<QJsonObject> ProjectManager::getAllProjectsAsJson() const
{
    QMutexLocker locker(&m_projectsMutex);

    QVector<QJsonObject> result;
    result.reserve(m_projects.size());

    for (const auto& project : m_projects) {
        if (project) {
            result.append(saveProjectAsJson(project));
        }
    }

    return result;
}

// === GUI Compatibility Interface ===

QVector<QWeakPointer<DataClass>> ProjectManager::getAllProjects() const
{
    QMutexLocker locker(&m_projectsMutex);

    QVector<QWeakPointer<DataClass>> result;
    result.reserve(m_projects.size());

    for (const auto& project : m_projects) {
        if (project) {
            result.append(project.toWeakRef());
        }
    }

    return result;
}

QHash<QString, QWeakPointer<DataClass>> ProjectManager::getProjectHash() const
{
    QMutexLocker locker(&m_projectsMutex);
    return m_projectHash;
}

int ProjectManager::getProjectCount() const
{
    QMutexLocker locker(&m_projectsMutex);
    return m_projects.size();
}

// === Model Access API Implementation - Claude Generated ===

QSharedPointer<AbstractModel> ProjectManager::getModel(const QString& projectId, const QString& modelId) const
{
    QMutexLocker locker(&m_projectsMutex);

#ifdef DEBUG_ON
    qDebug() << "ProjectManager::getModel: Looking for model" << modelId << "in project" << projectId;
#endif

    auto projectIt = m_projectHash.find(projectId);
    if (projectIt == m_projectHash.end()) {
        return QSharedPointer<AbstractModel>();
    }

    QSharedPointer<DataClass> project = projectIt.value().toStrongRef();
    if (!project) {
        return QSharedPointer<AbstractModel>();
    }

    // Iterate through project children (models are stored as children)
    for (int i = 0; i < project->ChildrenSize(); ++i) {
        QPointer<DataClass> child = project->Children(i);
        if (child && child->UUID() == modelId) {
#ifdef DEBUG_ON
            qDebug() << "ProjectManager::getModel: Found model" << modelId;
#endif
            // Return the real owning shared pointer so the caller participates in refcounting.
            QSharedPointer<AbstractModel> model = project->SharedModel(i);
            if (model)
                return model;
            // Fallback (should not happen for addModel()-registered models): non-owning handle.
            if (AbstractModel* modelPtr = dynamic_cast<AbstractModel*>(child.data()))
                return QSharedPointer<AbstractModel>(modelPtr, [](AbstractModel*) { /* no-op deleter */ });
        }
    }

    return QSharedPointer<AbstractModel>();
}

QVector<QSharedPointer<AbstractModel>> ProjectManager::getProjectModels(const QString& projectId) const
{
    QMutexLocker locker(&m_projectsMutex);
    QVector<QSharedPointer<AbstractModel>> models;

#ifdef DEBUG_ON
    qDebug() << "DEBUG ProjectManager::getProjectModels: Getting all models for project" << projectId;
#endif

    auto projectIt = m_projectHash.find(projectId);
    if (projectIt == m_projectHash.end()) {
        return models;
    }

    QSharedPointer<DataClass> project = projectIt.value().toStrongRef();
    if (!project) {
        return models;
    }

#ifdef DEBUG_ON
    qDebug() << "DEBUG ProjectManager::getProjectModels: Project has" << project->ChildrenSize() << "children";
#endif

    // Collect all models from project children
    for (int i = 0; i < project->ChildrenSize(); ++i) {
        QPointer<DataClass> child = project->Children(i);
        if (!child)
            continue;
        // Prefer the real owning shared pointer (correct refcounted ownership); only fall back
        // to a non-owning handle for a child that was never registered via addModel().
        QSharedPointer<AbstractModel> model = project->SharedModel(i);
        if (!model) {
            AbstractModel* modelPtr = dynamic_cast<AbstractModel*>(child.data());
            if (!modelPtr)
                continue;
            model = QSharedPointer<AbstractModel>(modelPtr, [](AbstractModel*) {});
        }
#ifdef DEBUG_ON
        qDebug() << "DEBUG ProjectManager::getProjectModels: Model" << i << "Name:" << model->Name() << "ModelUUID:" << model->ModelUUID();
#endif
        models.append(model);
    }

#ifdef DEBUG_ON
    qDebug() << "DEBUG ProjectManager::getProjectModels: Found" << models.size() << "models in project" << projectId;
#endif
    return models;
}

QStringList ProjectManager::getModelIds(const QString& projectId) const
{
    QMutexLocker locker(&m_projectsMutex);
    QStringList modelIds;
    
    auto projectIt = m_projectHash.find(projectId);
    if (projectIt == m_projectHash.end()) {
        return modelIds;
    }

    QSharedPointer<DataClass> project = projectIt.value().toStrongRef();
    if (!project) {
        return modelIds;
    }
    
    // Collect UUIDs from all model children
    for (int i = 0; i < project->ChildrenSize(); ++i) {
        QPointer<DataClass> child = project->Children(i);
        if (child) {
            AbstractModel* modelPtr = dynamic_cast<AbstractModel*>(child.data());
            if (modelPtr) {
                modelIds.append(child->UUID());
            }
        }
    }

#ifdef DEBUG_ON
    qDebug() << "DEBUG ProjectManager::getModelIds: Found" << modelIds.size() << "model IDs:" << modelIds;
#endif
    return modelIds;
}

int ProjectManager::getModelCount(const QString& projectId) const
{
    QMutexLocker locker(&m_projectsMutex);
    
    auto projectIt = m_projectHash.find(projectId);
    if (projectIt == m_projectHash.end()) {
        return -1;
    }
    
    QSharedPointer<DataClass> project = projectIt.value().toStrongRef();
    if (!project) {
        return -1;
    }
    
    int modelCount = 0;
    for (int i = 0; i < project->ChildrenSize(); ++i) {
        QPointer<DataClass> child = project->Children(i);
        if (child) {
            AbstractModel* modelPtr = dynamic_cast<AbstractModel*>(child.data());
            if (modelPtr) {
                modelCount++;
            }
        }
    }

#ifdef DEBUG_ON
    qDebug() << "DEBUG ProjectManager::getModelCount: Project" << projectId << "has" << modelCount << "models";
#endif
    return modelCount;
}

// === Utility Methods ===

void ProjectManager::clearAllProjects()
{
    QMutexLocker locker(&m_projectsMutex);

    m_projects.clear();
    m_projectHash.clear();
    m_metaBases.clear();
    m_currentProjectId.clear();

    emit projectsCleared();

#ifdef DEBUG_ON
    SFDebugPrint("✅ DEBUG ProjectManager::clearAllProjects: All projects cleared\n");
#endif
}

bool ProjectManager::hasProject(const QString& projectId) const
{
    QMutexLocker locker(&m_projectsMutex);
    return m_projectHash.contains(projectId);
}

QString ProjectManager::generateProjectId() const
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

// === Private Helper Methods ===

/*! \brief Creates a project from its JSON and tells in `lastLoadReport()` what was left out (Claude Generated). */
QString ProjectManager::loadProjectFromJson(const QJsonObject& jsonData, const QString& sourceFile)
{
    const bool own = !m_reportOpen; // `loadProject` has opened the report already
    if (own) {
        m_report = LoadReport();
        m_report.file = sourceFile;
    }
    const QString id = loadProjectFromJsonImpl(jsonData, sourceFile);
    m_report.projectId = id;
    if (own) {
        m_report.loaded = !id.isEmpty();
        if (id.isEmpty() && std::none_of(m_report.issues.begin(), m_report.issues.end(), [](const LoadIssue& i) { return i.severity == LoadIssue::Error; }))
            addIssue(LoadIssue::Error, "data", tr("The project could not be created."));
    }
    return id;
}

void ProjectManager::addIssue(LoadIssue::Severity severity, const QString& scope, const QString& message)
{
    m_report.issues.append(LoadIssue{ severity, m_scope.isEmpty() ? scope : m_scope + QStringLiteral("/") + scope, message });
}

QString ProjectManager::loadProjectFromJsonImpl(const QJsonObject& jsonData, const QString& sourceFile)
{
    LoadTiming::Scope loadTiming("pm.loadProjectFromJson");
    try {
        // Phase 2: Complete JSON handling - Extract data part for DataClass creation
        QJsonObject dataObject = jsonData.contains("data") ? jsonData["data"].toObject() : jsonData;
        // The data of the oldest files are stored otherwise: they go through the legacy reader; the current reader gets an empty object first,
        // the constructor reads what it is given.
        const int dataVersion = dataObject["SupraFit"].toInt();
        // Tables in "independent"/"dependent" the current reader takes whatever the version; only the first layout (strings of numbers under
        // "concentrations" and "signals") needs the legacy reader.
        const bool legacyData = !dataObject.contains("independent") && dataObject.contains("concentrations");
        QSharedPointer<DataClass> project = QSharedPointer<DataClass>::create(legacyData ? QJsonObject() : dataObject);
        if (legacyData && !project->LegacyImportData(dataObject, true))
            addIssue(LoadIssue::Warning, "data", tr("The data of the file (version %1) could not be read completely.").arg(dataVersion));

        if (!project) {
            addIssue(LoadIssue::Error, "data", tr("The data of the project could not be created."));
            emit errorOccurred("loadProjectFromJson", "Failed to create DataClass from JSON");
            return QString();
        }
        m_report.fileVersion = dataObject["SupraFit"].toInt();
        if (m_report.fileVersion > qint_version)
            addIssue(LoadIssue::Warning, "file", tr("The file was saved by a newer SupraFit (file version %1, this is %2); what this version does not know is left out.").arg(m_report.fileVersion).arg(qint_version));

        // Generate UUID if not present
        if (project->UUID().isEmpty()) {
            project->NewUUID();
        }

        QString projectId = project->UUID();

        // Extra top-level blocks (e.g. "agent_chats") stay with the project, opaque to the core,
        // and are written back on save. Claude Generated (WP11).
        QJsonObject attachments;
        if (jsonData.contains("data")) {
            for (auto it = jsonData.begin(); it != jsonData.end(); ++it) {
                if (it.key() != QLatin1String("data") && !it.key().startsWith(QLatin1String("model_")))
                    attachments.insert(it.key(), it.value());
            }
        }
        m_attachments[projectId] = attachments;
        m_unsavedAttachments.remove(projectId);

        // Phase 2: Load all model_X keys and create associated models
        int modelCount = 0;
        struct Reproduce {
            QString key;
            QSharedPointer<AbstractModel> model;
            double stored;
        };
        QList<Reproduce> reproduce;
#ifdef DEBUG_ON
        SFDebugPrint("DEBUG ProjectManager: Scanning for models in JSON keys: {}\n",
            jsonData.keys().join(", ").toStdString());
#endif

        for (const QString& key : jsonData.keys()) {
            if (key.startsWith("model_")) {
                ++m_report.modelsInFile;
#ifdef DEBUG_ON
                SFDebugPrint("DEBUG ProjectManager: Processing model key: {}\n", key.toStdString());
#endif
                // Claude Generated FIX 2: Type-check before conversion to ensure valid model data
                QJsonValue modelValue = jsonData[key];
                if (!modelValue.isObject()) {
                    addIssue(LoadIssue::Warning, key, tr("Left out: the entry is not a model (a JSON %1 instead of an object).").arg(modelValue.isArray() ? tr("array") : tr("value")));
                    emit errorOccurred("loadProjectFromJson",
                        QString("Model %1 has invalid type (expected object, got %2)")
                            .arg(key)
                            .arg(modelValue.type() == QJsonValue::Array ? "array" : "unknown"));
                    continue;
                }

                QJsonObject modelObject = modelValue.toObject();

                if (!modelObject.contains("model")) {
                    addIssue(LoadIssue::Warning, key, tr("Left out: the entry has no \"model\" field that says which model it is."));
                    emit errorOccurred("loadProjectFromJson",
                        QString("Model object %1 missing 'model' field").arg(key));
                    continue;
                }
                
                try {
                    // Create model using the factory function
                    // the oldest files name the model ("1:1-Model"), the others number it
                    const int modelId = modelObject["model"].isString() ? int(Name2Model(modelObject["model"].toString())) : modelObject["model"].toInt();

                    const int modelVersion = modelObject["SupraFit"].toInt();
#ifdef DEBUG_ON
                    SFDebugPrint("DEBUG ProjectManager: Creating model ID {} for project {}\n",
                        modelId, project->UUID().toStdString());
#endif

                    QSharedPointer<AbstractModel> model = CreateModel(modelId, project.data());
#ifdef DEBUG_ON
                    SFDebugPrint("DEBUG ProjectManager: CreateModel returned model: {}\n",
                        model ? "SUCCESS" : "FAILED");
#endif

                    if (model) {
#ifdef DEBUG_ON
                        qDebug() << "DEBUG ProjectManager: Model created with UUID:" << model->ModelUUID() << "for model name:" << model->Name();
#endif
                        // Import model state from JSON; what the model says on the way (a mismatch, a missing field) belongs to the report
                        QStringList said, warned;
                        const QMetaObject::Connection onWarning = QObject::connect(model->Info(), &DataClassPrivateObject::Warning, model.data(), [&said, &warned](const QString& text) { said << text; warned << text; });
                        const QMetaObject::Connection onMessage = QObject::connect(model->Info(), &DataClassPrivateObject::Message, model.data(), [&said](const QString& text) { said << text; });
                        bool importSuccess = (modelVersion < 2002) ? model->LegacyImportModel(modelObject) : model->ImportModel(modelObject);
                        QObject::disconnect(onWarning);
                        QObject::disconnect(onMessage);
                        said.removeDuplicates();
#ifdef DEBUG_ON
                        qDebug() << "DEBUG ProjectManager: After ImportModel, UUID:" << model->ModelUUID() << "ImportSuccess:" << importSuccess;
#endif
                        if (importSuccess) {
                            // the stored fit must come out of the stored parameters: checked after all models of the project are read
                            const QJsonValue storedValue = modelObject.contains("SSE") ? modelObject["SSE"] : modelObject["sum_of_squares"];
                            if (storedValue.isDouble() && storedValue.toDouble() > 0)
                                reproduce.append(Reproduce{ key, model, storedValue.toDouble() });
                            // Claude Generated - Remove incorrect duplicate detection
                            // Models loaded from JSON are unique by definition (model_1, model_2, etc.)
                            // Each JSON model key represents a unique model to be loaded
                            
                            // Claude Generated - Priority 2A: Add model to DataClass children list
                            // This makes the model visible in ProjectTree via project->Children(i)
                            project->addModel(model);
                            
                            modelCount++;
                            ++m_report.modelsLoaded;
                            for (const QString& text : std::as_const(said))
                                addIssue(warned.contains(text) ? LoadIssue::Warning : LoadIssue::Note, key, text);
                            // Emit signal for each successfully loaded model - CRITICAL FIX: Use ModelUUID() not UUID()
                            emit modelAdded(projectId, model->ModelUUID());

#ifdef DEBUG_ON
                            qDebug() << "DEBUG ProjectManager: Model" << model->Name() << "registered with ModelUUID:" << model->ModelUUID() << "Project now has" << project->ChildrenSize() << "children";
#endif
                        } else {
                            addIssue(LoadIssue::Warning, key, tr("Left out: the model \"%1\" could not be read from the file%2").arg(model->Name(), said.isEmpty() ? QStringLiteral(".") : QStringLiteral(": ") + said.join(" ")));
                            emit errorOccurred("loadProjectFromJson", 
                                QString("Failed to import model data for %1").arg(key));
                        }
                    } else {
                        if (ModelIdKnown(modelId))
                            addIssue(LoadIssue::Warning, key, tr("Left out: the model (number %1) does not fit the data of the project: it needs another number of independent columns.").arg(modelId));
                        else
                            addIssue(LoadIssue::Warning, key, tr("Left out: model number %1 is not known to this SupraFit (saved by a newer version, or the model has been removed).").arg(modelId));
                        emit errorOccurred("loadProjectFromJson", 
                            QString("Failed to create model %1 (ID: %2)").arg(key).arg(modelId));
                    }
                    
                } catch (const std::exception& e) {
                    addIssue(LoadIssue::Warning, key, tr("Left out: an error while reading the model: %1").arg(e.what()));
                    emit errorOccurred("loadProjectFromJson", 
                        QString("Exception loading model %1: %2").arg(key, e.what()));
                }
            }
        }


        // The fit as stored must come out of the stored parameters. If it does not, the model has changed since the file was written (or its
        // equation cannot be calculated here): the model is there, its numbers are not those of the file. Older fits often left out the
        // first points of the data (the first injection of a titration): if the same first points left out make the stored fits come out, the
        // data range of the project is set so.
        auto deviation = [](const Reproduce& r) {
            double spread = 0; // the scale of an SSE: the data around their column means; below a millionth of it an SSE is rounding
            const DataTable* dependent = r.model->DependentModel();
            for (int c = 0; c < dependent->columnCount(); ++c) {
                double mean = 0;
                for (int row = 0; row < dependent->rowCount(); ++row)
                    mean += dependent->data(row, c) / dependent->rowCount();
                for (int row = 0; row < dependent->rowCount(); ++row)
                    spread += (dependent->data(row, c) - mean) * (dependent->data(row, c) - mean);
            }
            const double now = r.model->SSE();
            if (!std::isfinite(now))
                return std::numeric_limits<double>::infinity();
            return (now - r.stored) / std::max(r.stored, 1e-6 * spread); // positive: worse than the file says
        };
        auto matching = [&](double tolerance) {
            int n = 0;
            for (const Reproduce& r : std::as_const(reproduce))
                n += std::abs(deviation(r)) <= tolerance;
            return n;
        };
        int firstSkipped = 0;
        if (!reproduce.isEmpty() && !project->isSimulation() && matching(1e-3) < reproduce.size()) {
            // leave out the first rows one more at a time (the check marks of the data tables, as the data table of the GUI does), keep the
            // number that makes the most stored fits come out exactly, and put every mark back if none does better than the rows as they are
            const Eigen::MatrixXd independentMarks = project->IndependentModel()->CheckedTable(), dependentMarks = project->DependentModel()->CheckedTable();
            const int rows = int(std::min(independentMarks.rows(), dependentMarks.rows())); // a simulation has no tables
            auto restore = [&](int skip) {
                Eigen::MatrixXd independent = independentMarks, dependent = dependentMarks;
                for (int row = 0; row < skip && row < rows; ++row) {
                    independent.row(row).setZero();
                    dependent.row(row).setZero();
                }
                project->IndependentModel()->setCheckedTable(independent);
                project->DependentModel()->setCheckedTable(dependent);
                for (const Reproduce& r : std::as_const(reproduce))
                    r.model->Calculate();
            };
            int best = matching(1e-6), bestSkip = 0;
            for (int skip = 1; skip <= 4 && skip < rows - 3; ++skip) {
                restore(skip);
                const int n = matching(1e-6);
                if (n > best) {
                    best = n;
                    bestSkip = skip;
                }
            }
            restore(bestSkip);
            firstSkipped = bestSkip;
            if (firstSkipped > 0)
                addIssue(LoadIssue::Note, "data", tr("The fits of the file leave out the first %1 data points; they are unchecked in the data table (check them there to use them).").arg(firstSkipped));
        }
        for (const Reproduce& r : std::as_const(reproduce)) {
            const double d = deviation(r);
            if (std::abs(d) <= 1e-3)
                continue;
            const double now = r.model->SSE();
            if (d > 0)
                addIssue(LoadIssue::Warning, r.key, tr("Read, but the fit of the file is not reproduced: the stored parameters give a sum of squares of %1 here, the file says %2. The model may have changed since the file was written; fit it again before using its numbers.").arg(now, 0, 'g', 4).arg(r.stored, 0, 'g', 4));
            else // lower: the model finds a better description of the same parameters (linear parameters are projected out)
                addIssue(LoadIssue::Note, r.key, tr("The sum of squares of the model is %1 here, the file says %2.").arg(now, 0, 'g', 4).arg(r.stored, 0, 'g', 4));
        }

        // Store source file info for debugging
        if (!sourceFile.isEmpty()) {
#ifdef DEBUG_ON
            qDebug() << "ProjectManager: Loaded complete project from:" << sourceFile 
                     << "UUID:" << projectId << "Models:" << modelCount;
#endif
        }

        // Claude Generated - CRITICAL FIX: Check if project already exists before appending
        if (m_projectHash.contains(projectId)) {
            QSharedPointer<DataClass> existingProject = m_projectHash[projectId].toStrongRef();
            if (existingProject) {
#ifdef DEBUG_ON
                qDebug() << "DEBUG ProjectManager: Project" << projectId << "already exists";
                qDebug() << "DEBUG ProjectManager: OLD project has" << existingProject->ChildrenSize() << "children";
                qDebug() << "DEBUG ProjectManager: NEW project has" << project->ChildrenSize() << "children";
#endif

                // If the new project has more models, replace the old one - Claude Generated
                if (project->ChildrenSize() > existingProject->ChildrenSize()) {
#ifdef DEBUG_ON
                    qDebug() << "DEBUG ProjectManager: Replacing old project with new one that has more models";
#endif

                    // Find and replace the old project in m_projects list
                    for (int i = 0; i < m_projects.size(); ++i) {
                        if (m_projects[i] && m_projects[i]->UUID() == projectId) {
                            m_projects[i] = project;
                            break;
                        }
                    }
                    
                    // Update the hash with new project
                    m_projectHash[projectId] = project.toWeakRef();
                    
                    // Emit signals for the updated project
                    emit projectAdded(projectId, project->ProjectTitle());
                    emit projectLoaded(projectId, sourceFile);
                } else {
                    // Keep existing project, just emit signals for GUI sync
                    emit projectAdded(projectId, existingProject->ProjectTitle());
                    emit projectLoaded(projectId, sourceFile);
                }
            }
            
            // Set as current project if none exists
            if (m_currentProjectId.isEmpty()) {
                m_currentProjectId = projectId;
                emit currentProjectChanged(projectId);
            }
            
            return projectId;  // Return existing project ID
        }

#ifdef DEBUG_ON
        qDebug() << "DEBUG ProjectManager: Adding NEW project with" << project->ChildrenSize() << "children to list";
#endif
        m_projects.append(project);
        updateProjectHash(); // Essential for GUI integration - Claude Generated

        // Emit signals for GUI integration - Claude Generated
        emit projectAdded(projectId, project->ProjectTitle());
        
        // CRITICAL FIX: Emit projectLoaded signal for GUI synchronization - Claude Generated
        emit projectLoaded(projectId, sourceFile);
        
        // Set as current project if none exists - Claude Generated
        if (m_currentProjectId.isEmpty()) {
            m_currentProjectId = projectId;
            emit currentProjectChanged(projectId);
        }

        return projectId;

    } catch (const std::exception& e) {
        addIssue(LoadIssue::Error, "data", tr("An error while reading the project: %1").arg(e.what()));
        emit errorOccurred("loadProjectFromJson", QString("Exception during JSON import: %1").arg(e.what()));
        return QString();
    }
}

bool ProjectManager::isMetaModel(const QString& projectId) const
{
    QMutexLocker locker(&m_projectsMutex);
    const QSharedPointer<DataClass> project = m_projectHash.value(projectId).toStrongRef();
    return project && qobject_cast<::MetaModel*>(project.data());
}

QSharedPointer<DataClass> ProjectManager::metaModelBase(const QString& projectId) const
{
    QMutexLocker locker(&m_projectsMutex);
    return m_metaBases.value(projectId);
}

QString ProjectManager::restoreMetaModel(const QJsonObject& block)
{
    LoadTiming::Scope loadTiming("pm.restoreMetaModel");
    const QString name = block["name"].toString();
    const QString label = name.isEmpty() ? tr("MetaModel") : name;
    const int size = block["size"].toInt();
    const QJsonObject uuids = block["uuids"].toObject(), raw = block["raw"].toObject();

    QSharedPointer<DataClass> base = QSharedPointer<DataClass>::create();
    base->setProjectTitle(label);
    if (base->UUID().isEmpty())
        base->NewUUID();
    QSharedPointer<AbstractModel> created = CreateModel(SupraFit::MetaModel, base);
    ::MetaModel* meta = qobject_cast<::MetaModel*>(created.data());
    if (!meta) {
        addIssue(LoadIssue::Warning, "file", tr("Left out: the meta model \"%1\" could not be created.").arg(label));
        return QString();
    }

    int found = 0;
    for (int i = 0; i < size; ++i) {
        const QString uuid = uuids[QString::number(i)].toString();
        const QSharedPointer<DataClass> source = m_projectHash.value(uuid).toStrongRef();
        const QJsonObject modelBlock = raw[QString::number(i)].toObject();
        if (!source) {
            addIssue(LoadIssue::Warning, "file", tr("The meta model \"%1\" lacks its model %2: the project %3 it belongs to is not in the file.").arg(label).arg(i + 1).arg(uuid));
            continue;
        }
        const int id = modelBlock["model"].isString() ? int(Name2Model(modelBlock["model"].toString())) : modelBlock["model"].toInt();
        QSharedPointer<AbstractModel> model = CreateModel(id, source.data());
        if (!model) {
            addIssue(LoadIssue::Warning, "file", tr("The meta model \"%1\" lacks its model %2: model number %3 does not fit the data of its project.").arg(label).arg(i + 1).arg(id));
            continue;
        }
        const bool imported = modelBlock["SupraFit"].toInt() < 2002 ? model->LegacyImportModel(modelBlock) : model->ImportModel(modelBlock);
        if (!imported) {
            addIssue(LoadIssue::Warning, "file", tr("The meta model \"%1\" lacks its model %2: it could not be read.").arg(label).arg(i + 1));
            continue;
        }
        meta->addModel(model.data()); // the meta model keeps a copy
        source->RemoveChild(model.data()); // the model made here is not a model of the project
        ++found;
    }
    if (!found) {
        addIssue(LoadIssue::Warning, "file", tr("Left out: the meta model \"%1\": none of its models is in the file.").arg(label));
        return QString();
    }
    meta->ImportModel(block);
    meta->setProjectTitle(label);

    QSharedPointer<DataClass> project = created.staticCast<DataClass>();
    const QString projectId = project->UUID();
    m_projects.append(project);
    m_projectHash[projectId] = project.toWeakRef();
    m_metaBases[projectId] = base;
    addIssue(LoadIssue::Note, "file", tr("The meta model \"%1\" was rebuilt from %2 of %3 models.").arg(label).arg(found).arg(size));
    emit projectAdded(projectId, label);
    return projectId;
}

QJsonObject ProjectManager::saveProjectAsJson(QSharedPointer<DataClass> project) const
{
    if (!project) {
        return QJsonObject();
    }

    try {
        // Wrap project data in correct JSON structure for GUI compatibility - Claude Generated
        QJsonObject wrappedProject;
        if (::MetaModel* meta = qobject_cast<::MetaModel*>(project.data())) {
            // a meta model is stored as a project whose "data" is the model itself (it names the projects of its models by UUID), as it always was
            wrappedProject["data"] = meta->ExportModel();
            return wrappedProject;
        }
        wrappedProject["data"] = project->ExportData();
        // Include serialized models (model_0, model_1, ...) so the JSON view exposes the
        // full project, matching the structure loadProjectFromJson() reads back - Claude Generated
        const QJsonObject models = project->ExportChildren();
        for (auto it = models.begin(); it != models.end(); ++it)
            wrappedProject[it.key()] = it.value();
        // attachments (agent chats, blocks of newer versions) - Claude Generated (WP11)
        const QJsonObject attachments = m_attachments.value(project->UUID());
        for (auto it = attachments.begin(); it != attachments.end(); ++it) {
            if (!wrappedProject.contains(it.key()))
                wrappedProject[it.key()] = it.value();
        }
        return wrappedProject;
    } catch (const std::exception& e) {
        // Log error without emit in const method
        qCritical() << "ProjectManager::saveProjectAsJson - Exception during JSON export:" << e.what();
        return QJsonObject();
    }
}

// === Current Project Management Implementation - Claude Generated ===

QJsonObject ProjectManager::getProjectDisplayInfo(const QString& projectId) const
{
    QMutexLocker locker(&m_projectsMutex);
    
    QString targetProjectId = projectId.isEmpty() ? m_currentProjectId : projectId;
    
    if (targetProjectId.isEmpty()) {
        return QJsonObject();
    }
    
    auto projectIt = m_projectHash.find(targetProjectId);
    if (projectIt == m_projectHash.end()) {
        return QJsonObject();
    }
    
    QSharedPointer<DataClass> project = projectIt.value().toStrongRef();
    if (!project) {
        return QJsonObject();
    }
    
    QJsonObject displayInfo;
    displayInfo["uuid"] = project->UUID();
    displayInfo["title"] = project->ProjectTitle();
    displayInfo["dataPoints"] = project->DataPoints();
    displayInfo["seriesCount"] = project->SeriesCount();
    displayInfo["independentCols"] = project->IndependentModel() ? project->IndependentModel()->columnCount() : 0;
    displayInfo["dependentCols"] = project->DependentModel() ? project->DependentModel()->columnCount() : 0;
    
    return displayInfo;
}

void ProjectManager::updateProjectHash()
{
    m_projectHash.clear();

    for (const auto& project : m_projects) {
        if (project && !project->UUID().isEmpty()) {
#ifdef DEBUG_ON
            qDebug() << "DEBUG ProjectManager::updateProjectHash: Adding project" << project->UUID() << "with" << project->ChildrenSize() << "children to hash";
#endif
            m_projectHash.insert(project->UUID(), project.toWeakRef());
        }
    }

#ifdef DEBUG_ON
    SFDebugPrint("DEBUG ProjectManager::updateProjectHash: Updated hash with {} projects\n", m_projectHash.size());
#endif
}

bool ProjectManager::validateProjectJson(const QJsonObject& projectJson) const
{
    return projectJsonProblem(projectJson).isEmpty();
}

/*! \brief Why the JSON is not a project we can read, empty if it is one (Claude Generated). */
QString ProjectManager::projectJsonProblem(const QJsonObject& projectJson) const
{
    if (projectJson.isEmpty())
        return tr("The file holds no data.");

    // wrapped project structure: "data" and the models next to it
    if (projectJson.contains("data")) {
        if (!projectJson["data"].isObject())
            return tr("The \"data\" entry of the file is not an object.");
        if (projectJson["data"].toObject().isEmpty())
            return tr("The \"data\" entry of the file is empty: the measured data of the project are missing.");
        return QString();
    }

    // direct DataClass structure (raw format)
    if (projectJson.contains("datatype"))
        return QString();

    // CLI configuration format (Main, Independent, Dependent structure)
    if (projectJson.contains("Main") || (projectJson.contains("Independent") && projectJson.contains("Dependent")))
        return QString();

    // Multi-Project format
    if (projectJson.contains("format_version") && projectJson["format_version"].toString() == "2.0") {
        for (auto it = projectJson.begin(); it != projectJson.end(); ++it) {
            if (it.key().startsWith("project_")) {
                QJsonObject projectData = it.value().toObject();
                if (!projectData.isEmpty() && (projectData.contains("data") || projectData.contains("datatype")))
                    return QString();
            }
        }
        return tr("The multi-project file contains no project with data.");
    }

    // files of several projects as SupraFit has always written them: "project_0", "project_1", ... without a version of their own
    for (auto it = projectJson.begin(); it != projectJson.end(); ++it)
        if (it.key().startsWith("project_") && it.value().isObject())
            return QString();

    QStringList keys = projectJson.keys();
    if (keys.size() > 6)
        keys = keys.mid(0, 6) << QStringLiteral("...");
    return tr("This is not a SupraFit project: there is no \"data\" entry (found: %1).").arg(keys.join(", "));
}

// Claude Generated - Backward compatibility method implementation
bool ProjectManager::registerExistingProject(QSharedPointer<DataClass> dataClass, const QString& filePath)
{
    if (!dataClass) {
        qWarning() << "ProjectManager::registerExistingProject: Null DataClass provided";
        return false;
    }

    QMutexLocker locker(&m_projectsMutex);
    
    QString projectId = dataClass->UUID();
    if (projectId.isEmpty()) {
        qWarning() << "ProjectManager::registerExistingProject: DataClass has empty UUID";
        return false;
    }

    // Check if project already exists
    if (m_projectHash.contains(projectId)) {
        qDebug() << "ProjectManager::registerExistingProject: Project already registered:" << projectId;
        return true;
    }

    try {
        // Register the DataClass
        m_projects.append(dataClass);
        m_projectHash[projectId] = dataClass.toWeakRef();
        
        // Store file path if provided (for future metadata tracking)
        // Note: m_projectFilePaths not implemented in header yet
        
        // Update current project if this is the first one
        if (m_currentProjectId.isEmpty()) {
            m_currentProjectId = projectId;
        }

        qDebug() << "ProjectManager::registerExistingProject: Successfully registered project"
                 << projectId << "with title:" << dataClass->ProjectTitle();

        // Emit signals for GUI updates
        QString projectTitle = dataClass->ProjectTitle();
        if (projectTitle.isEmpty()) {
            projectTitle = filePath.isEmpty() ? projectId : QFileInfo(filePath).baseName();
        }
        
        emit projectAdded(projectId, projectTitle);

        return true;
    } catch (const std::exception& e) {
        qWarning() << "ProjectManager::registerExistingProject: Exception during registration:" << e.what();
        return false;
    } catch (...) {
        qWarning() << "ProjectManager::registerExistingProject: Unknown exception during registration";
        return false;
    }
}

} // namespace SupraFit
namespace SupraFit {

bool LoadReport::HasProblems() const
{
    if (!loaded || modelsLoaded < modelsInFile || projectsLoaded < projectsInFile)
        return true;
    return std::any_of(issues.begin(), issues.end(), [](const LoadIssue& i) { return i.severity != LoadIssue::Note; });
}

QString LoadReport::Summary() const
{
    for (const LoadIssue& i : issues)
        if (i.severity == LoadIssue::Error)
            return i.message;
    if (fileVersion > qint_version) // the likely reason for everything else that is missing
        return QObject::tr("The file was saved by a newer SupraFit (file version %1, this is %2): what this version does not know is left out.").arg(fileVersion).arg(qint_version);
    if (projectsLoaded < projectsInFile)
        return QObject::tr("%1 of %2 projects of the file were read; the others are left out.").arg(projectsLoaded).arg(projectsInFile);
    if (modelsLoaded < modelsInFile)
        return QObject::tr("%1 of %2 models of the file were read; the others are left out.").arg(modelsLoaded).arg(modelsInFile);
    for (const LoadIssue& i : issues)
        if (i.severity == LoadIssue::Warning)
            return i.message;
    return QString();
}

QString LoadReport::Details() const
{
    QStringList lines;
    for (int severity = LoadIssue::Error; severity >= LoadIssue::Note; --severity)
        for (const LoadIssue& i : issues)
            if (i.severity == severity)
                lines << QStringLiteral("%1 [%2]: %3").arg(severity == LoadIssue::Error ? QObject::tr("Error") : severity == LoadIssue::Warning ? QObject::tr("Warning") : QObject::tr("Note"), i.scope, i.message);
    if (fileVersion > 0)
        lines << QObject::tr("File version: %1").arg(fileVersion);
    return lines.join('\n');
}

} // namespace SupraFit
