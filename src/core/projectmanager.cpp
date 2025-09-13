/*
 * SupraFit Project Manager - Centralized Project Management Implementation
 * Copyright (C) 2025 Conrad Hübler <Conrad.Huebler@gmx.net>
 *
 * Implementation of centralized project management for CLI/GUI integration
 * Claude Generated - Educational-First Design Implementation
 */

#include "projectmanager.h"

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
    QMutexLocker locker(&m_projectsMutex);

    if (filePath.isEmpty()) {
        emit errorOccurred("loadProject", "Empty file path provided");
        return false;
    }

    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        emit errorOccurred("loadProject", QString("File does not exist: %1").arg(filePath));
        return false;
    }

#ifdef DEBUG_ON
    fmt::print("🔍 DEBUG ProjectManager::loadProject: Loading file: {}\n", filePath.toStdString());
#endif

    try {
        // Use existing FileHandler infrastructure
        FileHandler* handler = new FileHandler(filePath, this);
        handler->LoadFile();

        QJsonObject projectData;
        if (handler->Type() == FileHandler::SupraFit) {
            // Load SupraFit project file
            if (!JsonHandler::ReadJsonFile(projectData, filePath)) {
                delete handler;
                emit errorOccurred("loadProject", QString("Failed to read SupraFit file: %1").arg(filePath));
                return false;
            }
        } else {
            // Use handler data for other formats
            projectData = handler->getJsonData();
        }

        delete handler;

        // Validate and create project
        if (!validateProjectJson(projectData)) {
            emit errorOccurred("loadProject", QString("Invalid project structure in file: %1").arg(filePath));
            return false;
        }

        // Claude Generated - Debug JSON data structure passed to loadProjectFromJson
        fmt::print("🔍 DEBUG loadProject: Passing JSON with keys {} to loadProjectFromJson\n", 
            [&projectData]() {
                QStringList keys;
                for (auto it = projectData.begin(); it != projectData.end(); ++it) {
                    keys << it.key();
                }
                return keys.join(", ").toStdString();
            }());
        
        QString projectId = loadProjectFromJson(projectData, filePath);
        if (projectId.isEmpty()) {
            emit errorOccurred("loadProject", QString("Failed to create project from file: %1").arg(filePath));
            return false;
        }

        // Set as current project if none is active
        if (m_currentProjectId.isEmpty()) {
            m_currentProjectId = projectId;
        }

        updateProjectHash();

        emit projectLoaded(projectId, filePath);

#ifdef DEBUG_ON
        fmt::print("✅ DEBUG ProjectManager::loadProject: Successfully loaded project {} from {}\n",
            projectId.toStdString(), filePath.toStdString());
#endif

        return true;

    } catch (const std::exception& e) {
        emit errorOccurred("loadProject", QString("Exception during project loading: %1").arg(e.what()));
        return false;
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
    fmt::print("🔍 DEBUG ProjectManager::saveProject: Saving project {} to {}\n",
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
            emit projectSaved(targetProjectId, filePath);

#ifdef DEBUG_ON
            fmt::print("✅ DEBUG ProjectManager::saveProject: Successfully saved project {} to {}\n",
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
    fmt::print("🔍 DEBUG ProjectManager::saveAllProjects: Saving {} projects to {}\n",
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
            fmt::print("✅ DEBUG ProjectManager::saveAllProjects: Successfully saved {} projects\n",
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
        fmt::print("✅ DEBUG ProjectManager::addModelToProject: Added model {} to project {}\n",
            model->ModelUUID().toStdString(), targetProjectId.toStdString());
#endif

        return true;

    } catch (const std::exception& e) {
        emit errorOccurred("addModelToProject", QString("Exception adding model: %1").arg(e.what()));
        return false;
    }
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

    emit projectRemoved(projectId);

#ifdef DEBUG_ON
    fmt::print("✅ DEBUG ProjectManager::removeProject: Removed project {}\n", projectId.toStdString());
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

    QString projectId = loadProjectFromJson(projectJson, projectTitle);
    if (!projectId.isEmpty()) {
        updateProjectHash();

        // Set as current project if none is active
        if (m_currentProjectId.isEmpty()) {
            m_currentProjectId = projectId;
        }

        emit projectAdded(projectId, projectTitle.isEmpty() ? "Generated Project" : projectTitle);

#ifdef DEBUG_ON
        fmt::print("✅ DEBUG ProjectManager::createProjectFromJson: Created project {} with title '{}'\n",
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
            // Cast DataClass back to AbstractModel (models inherit from DataClass)
            AbstractModel* modelPtr = dynamic_cast<AbstractModel*>(child.data());
            if (modelPtr) {
#ifdef DEBUG_ON
                qDebug() << "ProjectManager::getModel: Found model" << modelId;
#endif
                // Return QSharedPointer without taking ownership (model is managed by DataClass parent)
                return QSharedPointer<AbstractModel>(modelPtr, [](AbstractModel*) { /* no-op deleter */ });
            }
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
        if (child) {
            AbstractModel* modelPtr = dynamic_cast<AbstractModel*>(child.data());
            if (modelPtr) {
#ifdef DEBUG_ON
                qDebug() << "DEBUG ProjectManager::getProjectModels: Model" << i << "Name:" << modelPtr->Name() << "ModelUUID:" << modelPtr->ModelUUID();
#endif
                models.append(QSharedPointer<AbstractModel>(modelPtr, [](AbstractModel*) {}));
            }
        }
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
    m_currentProjectId.clear();

    emit projectsCleared();

#ifdef DEBUG_ON
    fmt::print("✅ DEBUG ProjectManager::clearAllProjects: All projects cleared\n");
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

QString ProjectManager::loadProjectFromJson(const QJsonObject& jsonData, const QString& sourceFile)
{
    try {
        // Phase 2: Complete JSON handling - Extract data part for DataClass creation
        QJsonObject dataObject = jsonData.contains("data") ? jsonData["data"].toObject() : jsonData;
        QSharedPointer<DataClass> project = QSharedPointer<DataClass>::create(dataObject);

        if (!project) {
            emit errorOccurred("loadProjectFromJson", "Failed to create DataClass from JSON");
            return QString();
        }

        // Generate UUID if not present
        if (project->UUID().isEmpty()) {
            project->NewUUID();
        }

        QString projectId = project->UUID();
        
        // Phase 2: Load all model_X keys and create associated models
        int modelCount = 0;
#ifdef DEBUG_ON
        fmt::print("DEBUG ProjectManager: Scanning for models in JSON keys: {}\n",
            jsonData.keys().join(", ").toStdString());
#endif

        for (const QString& key : jsonData.keys()) {
            if (key.startsWith("model_")) {
#ifdef DEBUG_ON
                fmt::print("DEBUG ProjectManager: Processing model key: {}\n", key.toStdString());
#endif
                QJsonObject modelObject = jsonData[key].toObject();
                
                if (!modelObject.contains("model")) {
                    emit errorOccurred("loadProjectFromJson", 
                        QString("Model object %1 missing 'model' field").arg(key));
                    continue;
                }
                
                try {
                    // Create model using the factory function
                    int modelId = modelObject["model"].toInt();
#ifdef DEBUG_ON
                    fmt::print("DEBUG ProjectManager: Creating model ID {} for project {}\n",
                        modelId, project->UUID().toStdString());
#endif

                    QSharedPointer<AbstractModel> model = CreateModel(modelId, project.data());
#ifdef DEBUG_ON
                    fmt::print("DEBUG ProjectManager: CreateModel returned model: {}\n",
                        model ? "SUCCESS" : "FAILED");
#endif

                    if (model) {
#ifdef DEBUG_ON
                        qDebug() << "DEBUG ProjectManager: Model created with UUID:" << model->ModelUUID() << "for model name:" << model->Name();
#endif
                        // Import model state from JSON
                        bool importSuccess = model->ImportModel(modelObject);
#ifdef DEBUG_ON
                        qDebug() << "DEBUG ProjectManager: After ImportModel, UUID:" << model->ModelUUID() << "ImportSuccess:" << importSuccess;
#endif
                        if (importSuccess) {
                            // Claude Generated - Remove incorrect duplicate detection
                            // Models loaded from JSON are unique by definition (model_1, model_2, etc.)
                            // Each JSON model key represents a unique model to be loaded
                            
                            // Claude Generated - Priority 2A: Add model to DataClass children list
                            // This makes the model visible in ProjectTree via project->Children(i)
                            project->addModel(model);
                            
                            modelCount++;
                            // Emit signal for each successfully loaded model - CRITICAL FIX: Use ModelUUID() not UUID()
                            emit modelAdded(projectId, model->ModelUUID());

#ifdef DEBUG_ON
                            qDebug() << "DEBUG ProjectManager: Model" << model->Name() << "registered with ModelUUID:" << model->ModelUUID() << "Project now has" << project->ChildrenSize() << "children";
#endif
                        } else {
                            emit errorOccurred("loadProjectFromJson", 
                                QString("Failed to import model data for %1").arg(key));
                        }
                    } else {
                        emit errorOccurred("loadProjectFromJson", 
                            QString("Failed to create model %1 (ID: %2)").arg(key).arg(modelId));
                    }
                    
                } catch (const std::exception& e) {
                    emit errorOccurred("loadProjectFromJson", 
                        QString("Exception loading model %1: %2").arg(key, e.what()));
                }
            }
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
        emit errorOccurred("loadProjectFromJson", QString("Exception during JSON import: %1").arg(e.what()));
        return QString();
    }
}

QJsonObject ProjectManager::saveProjectAsJson(QSharedPointer<DataClass> project) const
{
    if (!project) {
        return QJsonObject();
    }

    try {
        // Wrap project data in correct JSON structure for GUI compatibility - Claude Generated
        QJsonObject wrappedProject;
        wrappedProject["data"] = project->ExportData();
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
    fmt::print("DEBUG ProjectManager::updateProjectHash: Updated hash with {} projects\n", m_projectHash.size());
#endif
}

bool ProjectManager::validateProjectJson(const QJsonObject& projectJson) const
{
    // Basic validation - check for essential structure
    if (projectJson.isEmpty()) {
        return false;
    }

    // Check for wrapped project structure (legacy format)
    if (projectJson.contains("data")) {
        QJsonObject data = projectJson["data"].toObject();
        if (data.isEmpty()) {
            return false;
        }
        return true;
    }

    // Check for direct DataClass structure (raw format)
    // Must have datatype for DataClass validation
    if (projectJson.contains("datatype")) {
        // Optional checks for dependent/independent data
        return true;
    }

    // Check for CLI configuration format (Main, Independent, Dependent structure)
    if (projectJson.contains("Main") || (projectJson.contains("Independent") && projectJson.contains("Dependent"))) {
        // This is a CLI configuration format for data generation
        return true;
    }

    // If none of the formats is recognized, fail validation
    return false;
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