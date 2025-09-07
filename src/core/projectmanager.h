/*
 * SupraFit Project Manager - Centralized Project Management for CLI and GUI
 * Copyright (C) 2025 Conrad Hübler <Conrad.Huebler@gmx.net>
 *
 * This file implements centralized project management to eliminate code
 * duplication between CLI and GUI frontends by providing unified project
 * I/O, state management, and data access operations.
 *
 * Claude Generated - Educational-First Design Implementation
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "src/core/models/dataclass.h"
#include "src/core/models/models.h"

#include <QtCore/QHash>
#include <QtCore/QJsonObject>
#include <QtCore/QMutex>
#include <QtCore/QObject>
#include <QtCore/QSharedPointer>
#include <QtCore/QUuid>
#include <QtCore/QVector>
#include <QtCore/QWeakPointer>

namespace SupraFit {

/**
 * @brief Centralized Project Manager for CLI and GUI Integration
 *
 * The ProjectManager eliminates code duplication between SupraFitCli and
 * SupraFitGui by providing a unified interface for project operations.
 *
 * Key Features:
 * - Thread-safe singleton access for both CLI and GUI
 * - Unified project I/O operations (load/save)
 * - Signal-based notifications for GUI Model-View integration
 * - JSON and DataClass dual compatibility
 * - UUID-based project identification and caching
 *
 * Educational Design:
 * - Clear separation between CLI JSON operations and GUI DataClass operations
 * - Transparent project state management with comprehensive logging
 * - Atomic operations to prevent data corruption during concurrent access
 */
class ProjectManager : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Singleton instance access
     * Thread-safe singleton implementation for unified CLI/GUI access
     */
    static ProjectManager& instance();

    virtual ~ProjectManager();

    // === Core Project Management ===

    /**
     * @brief Load project from file path
     * @param filePath Path to project file (.suprafit, .json, etc.)
     * @return Success status of load operation
     *
     * Supports multiple file formats:
     * - .suprafit (compressed binary format)
     * - .json (human-readable format)
     * - .dat/.txt/.csv (data tables)
     * - .itc/.dh (specialized formats)
     */
    bool loadProject(const QString& filePath);

    /**
     * @brief Save project to file path
     * @param filePath Output file path
     * @param projectId UUID of project to save (optional, defaults to current)
     * @return Success status of save operation
     */
    bool saveProject(const QString& filePath, const QString& projectId = QString());

    /**
     * @brief Save all open projects to file
     * @param filePath Output file path for multi-project file
     * @return Success status of batch save operation
     */
    bool saveAllProjects(const QString& filePath);

    // === Project State Management ===

    /**
     * @brief Get currently active project
     * @return Weak pointer to active project, null if none active
     */
    QWeakPointer<DataClass> getCurrentProject() const;

    /**
     * @brief Set currently active project by UUID
     * @param projectId UUID of project to make active
     * @return Success status of operation
     */
    bool setCurrentProject(const QString& projectId);

    /**
     * @brief Get project by UUID
     * @param projectId UUID of requested project
     * @return Weak pointer to project, null if not found
     */
    QWeakPointer<DataClass> getProject(const QString& projectId) const;

    /**
     * @brief Add model to specified project
     * @param model Shared pointer to model to add
     * @param projectId Target project UUID (optional, defaults to current)
     * @return Success status of model addition
     */
    bool addModelToProject(QSharedPointer<AbstractModel> model, const QString& projectId = QString());

    /**
     * @brief Remove project from manager
     * @param projectId UUID of project to remove
     * @return Success status of removal
     */
    bool removeProject(const QString& projectId);

    // === CLI Compatibility Interface ===

    /**
     * @brief Get project as JSON object for CLI operations
     * @param projectId UUID of project (optional, defaults to current)
     * @return JSON representation of project data
     */
    QJsonObject getProjectAsJson(const QString& projectId = QString()) const;

    /**
     * @brief Create project from JSON object (CLI data generation)
     * @param projectJson JSON object containing project data
     * @param projectTitle Optional title for project identification
     * @return UUID of created project, empty string on failure
     */
    QString createProjectFromJson(const QJsonObject& projectJson, const QString& projectTitle = QString());

    /**
     * @brief Get all projects as JSON vector for CLI batch operations
     * @return Vector of JSON objects representing all projects
     */
    QVector<QJsonObject> getAllProjectsAsJson() const;

    // === GUI Compatibility Interface ===

    /**
     * @brief Get all projects for GUI Model-View integration
     * @return Vector of weak pointers to all managed projects
     */
    QVector<QWeakPointer<DataClass>> getAllProjects() const;

    /**
     * @brief Get project hash map for GUI UUID-based access
     * @return Hash map of UUID to project weak pointers
     */
    QHash<QString, QWeakPointer<DataClass>> getProjectHash() const;

    /**
     * @brief Get project count for GUI displays
     * @return Number of currently managed projects
     */
    int getProjectCount() const;

    // === Utility Methods ===

    /**
     * @brief Clear all projects from manager
     * Emits projectsCleared() signal for GUI updates
     */
    void clearAllProjects();

    /**
     * @brief Check if project exists in manager
     * @param projectId UUID of project to check
     * @return True if project exists, false otherwise
     */
    bool hasProject(const QString& projectId) const;

    /**
     * @brief Generate new unique project UUID
     * @return New UUID string for project identification
     */
    QString generateProjectId() const;

signals:
    /**
     * @brief Emitted when project is successfully loaded
     * @param projectId UUID of loaded project
     * @param filePath Path from which project was loaded
     */
    void projectLoaded(const QString& projectId, const QString& filePath);

    /**
     * @brief Emitted when project is successfully saved
     * @param projectId UUID of saved project
     * @param filePath Path to which project was saved
     */
    void projectSaved(const QString& projectId, const QString& filePath);

    /**
     * @brief Emitted when new project is added to manager
     * @param projectId UUID of added project
     * @param projectTitle Title of added project
     */
    void projectAdded(const QString& projectId, const QString& projectTitle);

    /**
     * @brief Emitted when project is removed from manager
     * @param projectId UUID of removed project
     */
    void projectRemoved(const QString& projectId);

    /**
     * @brief Emitted when model is added to project
     * @param projectId UUID of target project
     * @param modelId UUID of added model
     */
    void modelAdded(const QString& projectId, const QString& modelId);

    /**
     * @brief Emitted when current project changes
     * @param projectId UUID of new current project
     */
    void currentProjectChanged(const QString& projectId);

    /**
     * @brief Emitted when all projects are cleared
     */
    void projectsCleared();

    /**
     * @brief Emitted on project operation errors
     * @param operation Description of failed operation
     * @param errorMessage Detailed error information
     */
    void errorOccurred(const QString& operation, const QString& errorMessage);

private:
    /**
     * @brief Private constructor for singleton pattern
     */
    explicit ProjectManager(QObject* parent = nullptr);

    /**
     * @brief Load project from JSON data
     * @param jsonData JSON object containing project data
     * @param sourceFile Original file path for metadata
     * @return UUID of created project, empty string on failure
     */
    QString loadProjectFromJson(const QJsonObject& jsonData, const QString& sourceFile);

    /**
     * @brief Save project as JSON data
     * @param project Project to save
     * @return JSON object representation of project
     */
    QJsonObject saveProjectAsJson(QSharedPointer<DataClass> project) const;

    /**
     * @brief Update project hash cache after modifications
     */
    void updateProjectHash();

    /**
     * @brief Validate project JSON structure
     * @param projectJson JSON object to validate
     * @return True if valid project structure, false otherwise
     */
    bool validateProjectJson(const QJsonObject& projectJson) const;

    // === Member Variables ===

    // Project storage and management
    QVector<QSharedPointer<DataClass>> m_projects; ///< All managed projects
    QHash<QString, QWeakPointer<DataClass>> m_projectHash; ///< UUID-based project access
    QString m_currentProjectId; ///< Currently active project UUID

    // Thread safety and synchronization
    mutable QMutex m_projectsMutex; ///< Mutex for thread-safe access

    // Singleton instance management
    static QMutex s_instanceMutex; ///< Singleton creation mutex
    static ProjectManager* s_instance; ///< Singleton instance pointer

    // Disable copy constructor and assignment for singleton
    ProjectManager(const ProjectManager&) = delete;
    ProjectManager& operator=(const ProjectManager&) = delete;
};

} // namespace SupraFit