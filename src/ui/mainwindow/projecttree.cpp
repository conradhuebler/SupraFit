/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2020 - 2025 Conrad Hübler <Conrad.Huebler@gmx.net>
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
 *
 */

#include <QtCore/QJsonDocument>

#include "QtGui/QFont"

#include "src/core/models/dataclass.h"
#include "src/core/models/models.h"

#include "projecttree.h"

// Claude Generated - ProjectManager Integration Helper Implementation
QVector<QWeakPointer<DataClass>> ProjectTree::getUnifiedProjectList() const
{
    // Try to get projects from ProjectManager first
    SupraFit::ProjectManager& projectManager = SupraFit::ProjectManager::instance();
    QStringList managedProjectIds = projectManager.getLoadedProjectIds();
    
    if (!managedProjectIds.isEmpty()) {
        QVector<QWeakPointer<DataClass>> projects;
        for (const QString& projectId : managedProjectIds) {
            QSharedPointer<DataClass> dataClass = projectManager.getProjectData(projectId);
            if (dataClass) {
                projects.append(dataClass.toWeakRef());
            }
        }
        return projects;
    }
    
    // Fallback to m_data_list for backward compatibility
    if (m_data_list && !m_data_list->isEmpty()) {
        return *m_data_list;
    }
    
    return QVector<QWeakPointer<DataClass>>();
}

QSize ProjectTreeEntry::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    if (index.parent().isValid())
        return QSize(50, 25);
    else
        return QSize(100, 25);
}

// Claude Generated - ProjectManager Integration for UpdateStructure (CRASH FIX)
void ProjectTree::UpdateStructure()
{
    qDebug() << "🔍 DEBUG ProjectTree::UpdateStructure: Starting update";
    
    // Clear existing UUID mappings
    m_uuids.clear();
    m_ptr_uuids.clear();
    qDebug() << "🔍 DEBUG ProjectTree::UpdateStructure: Starting update";

    QVector<QWeakPointer<DataClass>> projectList = getUnifiedProjectList();
    qDebug() << "🔍 DEBUG ProjectTree::UpdateStructure: Processing" << projectList.size() << "projects";
    
    for (int i = 0; i < projectList.size(); ++i) {
        QSharedPointer<DataClass> project = projectList[i].toStrongRef();
        if (!project) {
            qDebug() << "🔍 DEBUG ProjectTree::UpdateStructure: Project" << i << "is null, skipping";
            continue;
        }
        
        QString uuid = project->UUID();
        qDebug() << "🔍 DEBUG ProjectTree::UpdateStructure: Processing project" << i << "UUID:" << uuid << "Title:" << project->ProjectTitle();

        // Add project UUID - Claude Generated - CRITICAL FIX: Top-level items need nullptr as internal pointer
        if (!m_uuids.contains(uuid)) {
            m_uuids << uuid;
            // Top-level projects use nullptr as internal pointer (Qt convention)
            m_ptr_uuids << nullptr;
            qDebug() << "🔍 DEBUG ProjectTree::UpdateStructure: Added project UUID to lists with nullptr pointer";
        }

        // CRITICAL FIX: Safe model processing with pointer-based child index counting
        int childrenCount = project->ChildrenSize();
        qDebug() << "🔍 DEBUG ProjectTree::UpdateStructure: Project has" << childrenCount << "children";
        
        // Claude Generated - Track pointer occurrence counts for proper child indexing
        QHash<QString, int> pointerOccurrenceCount;
        
        for (int j = 0; j < childrenCount; ++j) {
            qDebug() << "🔍 DEBUG ProjectTree::UpdateStructure: Processing child" << j;
            
            QPointer<DataClass> child = project->Children(j);
            if (!child) {
                qDebug() << "❌ DEBUG ProjectTree::UpdateStructure: Child" << j << "is null pointer, skipping";
                continue;
            }
            
            AbstractModel* model = qobject_cast<AbstractModel*>(child.data());
            if (!model) {
                qDebug() << "❌ DEBUG ProjectTree::UpdateStructure: Child" << j << "is not an AbstractModel, skipping";
                continue;
            }
            
            // Claude Generated - Calculate child index as pointer occurrence count (not absolute position)
            QString modelPointer = QString::number(reinterpret_cast<quintptr>(model), 16);
            int childIndex = pointerOccurrenceCount.value(modelPointer, 0);
            pointerOccurrenceCount[modelPointer] = childIndex + 1;
            
            QString sub_uuid = uuid + "|" + QString::number(childIndex) + "|" + modelPointer;
            qDebug() << "🔍 DEBUG ProjectTree::UpdateStructure: Child" << j << "ChildIndex:" << childIndex << "Pointer:" << modelPointer << "Sub-UUID:" << sub_uuid;

            // Claude Generated - Force addition for child index differentiation, even if pointer matches
            // This ensures that duplicate model instances with different child indices get unique UUIDs
            if (!m_uuids.contains(sub_uuid)) {
                m_uuids << sub_uuid;
                // Child items use index+1 as internal pointer (0 is reserved for top-level items)
                m_ptr_uuids << reinterpret_cast<void*>(m_uuids.size());
                qDebug() << "🔍 DEBUG ProjectTree::UpdateStructure: Added child UUID to lists with pointer index" << m_uuids.size();
            } else {
                // Claude Generated - Force unique UUID when duplicate pointer detected
                QString forced_uuid = uuid + "|" + QString::number(childIndex) + "|" + modelPointer + "|forced";
                qDebug() << "🔍 DEBUG ProjectTree::UpdateStructure: Sub-UUID collision, forcing unique:" << forced_uuid;
                m_uuids << forced_uuid;
                m_ptr_uuids << reinterpret_cast<void*>(m_uuids.size());
            }
        }
    }
    
    qDebug() << "🔍 DEBUG ProjectTree::UpdateStructure: Final UUID count:" << m_uuids.size() << "Calling layoutChanged()";
    
    // Claude Generated - Safe layoutChanged() call with proper model notifications
    try {
        beginResetModel();
        endResetModel();
        qDebug() << "🔍 DEBUG ProjectTree::UpdateStructure: Layout reset completed safely";
    } catch (...) {
        qDebug() << "❌ DEBUG ProjectTree::UpdateStructure: Layout reset failed, continuing";
    }
    
    qDebug() << "🔍 DEBUG ProjectTree::UpdateStructure: Update complete";
}

QString ProjectTree::UUID(const QModelIndex& index) const
{
    void* internalPtr = index.internalPointer();
    
    // Claude Generated - CRITICAL FIX: Handle top-level vs child items correctly
    if (!internalPtr) {
        // Top-level item: use row to find project UUID
        if (!index.parent().isValid() && index.row() >= 0) {
            QVector<QWeakPointer<DataClass>> projectList = getUnifiedProjectList();
            if (index.row() < projectList.size()) {
                QSharedPointer<DataClass> project = projectList[index.row()].toStrongRef();
                if (project) {
                    return project->UUID();
                }
            }
        }
        return QString();
    }
    
    // Child item: convert pointer back to index  
    int uuidIndex = reinterpret_cast<quintptr>(internalPtr) - 1; // Subtract 1 because we added 1 in UpdateStructure
    if (uuidIndex < 0 || uuidIndex >= m_uuids.size()) {
        qDebug() << "❌ DEBUG ProjectTree::UUID: Invalid UUID index" << uuidIndex << "/ valid range: 0-" << (m_uuids.size() - 1);
        return QString();
    }
    
    return m_uuids[uuidIndex];
}

int ProjectTree::columnCount(const QModelIndex& parent) const
{
    qDebug() << "🔍 DEBUG ProjectTree::columnCount: Called with parent valid:" << parent.isValid();
    
    if (parent.isValid()) {
        qDebug() << "🔍 DEBUG ProjectTree::columnCount: Parent valid, checking internalPointer";
        void* internalPtr = parent.internalPointer();
        qDebug() << "🔍 DEBUG ProjectTree::columnCount: Parent internalPtr:" << internalPtr;
        
        if (internalPtr == nullptr) {
            // Parent is a top-level project, children (models) have 1 column
            qDebug() << "🔍 DEBUG ProjectTree::columnCount: Parent is project, returning 1 column for children";
            return 1;
        } else {
            // Parent is a model, models don't have children
            qDebug() << "🔍 DEBUG ProjectTree::columnCount: Parent is model, returning 0 columns";
            return 0;
        }
    } else {
        qDebug() << "🔍 DEBUG ProjectTree::columnCount: No parent (top-level), returning 2 columns";
        return 2;
    }
}

// Claude Generated - ProjectManager Integration for rowCount
int ProjectTree::rowCount(const QModelIndex& p) const
{
    qDebug() << "🔍 DEBUG ProjectTree::rowCount: Called with valid index:" << p.isValid();
    
    QVector<QWeakPointer<DataClass>> projectList = getUnifiedProjectList();
    int count = projectList.size();
    
    qDebug() << "🔍 DEBUG ProjectTree::rowCount: Base projectList size:" << count;
    
    if (p.isValid()) {
        qDebug() << "🔍 DEBUG ProjectTree::rowCount: Processing valid index, row:" << p.row() << "column:" << p.column();
        
        // Claude Generated - CRITICAL FIX: Safe UUID access with proper error handling
        QString uuid;
        void* internalPtr = p.internalPointer();
        qDebug() << "🔍 DEBUG ProjectTree::rowCount: InternalPointer:" << internalPtr;
        
        if (!internalPtr) {
            // Top-level item: get UUID directly from project
            qDebug() << "🔍 DEBUG ProjectTree::rowCount: Top-level item, getting UUID from project at row" << p.row();
            if (p.row() >= 0 && p.row() < projectList.size()) {
                QSharedPointer<DataClass> project = projectList[p.row()].toStrongRef();
                if (project) {
                    uuid = project->UUID();
                    qDebug() << "🔍 DEBUG ProjectTree::rowCount: Got UUID from project:" << uuid;
                } else {
                    qDebug() << "❌ DEBUG ProjectTree::rowCount: Project pointer is null";
                    return 0;
                }
            } else {
                qDebug() << "❌ DEBUG ProjectTree::rowCount: Row out of bounds:" << p.row() << "/" << projectList.size();
                return 0;
            }
        } else {
            // Child item: get UUID from m_uuids array
            int uuidIndex = reinterpret_cast<quintptr>(internalPtr) - 1;
            qDebug() << "🔍 DEBUG ProjectTree::rowCount: Child item, UUID index:" << uuidIndex;
            if (uuidIndex >= 0 && uuidIndex < m_uuids.size()) {
                uuid = m_uuids[uuidIndex];
                qDebug() << "🔍 DEBUG ProjectTree::rowCount: Got UUID from m_uuids:" << uuid;
            } else {
                qDebug() << "❌ DEBUG ProjectTree::rowCount: UUID index out of bounds:" << uuidIndex << "/" << m_uuids.size();
                return 0;
            }
        }
        
        qDebug() << "🔍 DEBUG ProjectTree::rowCount: Final UUID:" << uuid << "length:" << uuid.size();
        
        if (uuid.size() >= 50) // Model Element (simplified structure: project_uuid|child_index|pointer_hex)
        {
            qDebug() << "🔍 DEBUG ProjectTree::rowCount: Model element (no children)";
            count = 0;
        } else if (uuid.size() == 38) // DataClass Element
        {
            qDebug() << "🔍 DEBUG ProjectTree::rowCount: DataClass element, checking children";
            if (p.row() < projectList.size()) {
                QSharedPointer<DataClass> project = projectList[p.row()].toStrongRef();
                if (project) {
                    qDebug() << "🔍 DEBUG ProjectTree::rowCount: Project valid, getting ChildrenSize()";
                    try {
                        count = project->ChildrenSize();
                        qDebug() << "🔍 DEBUG ProjectTree::rowCount: ChildrenSize() returned:" << count;
                    } catch (...) {
                        qDebug() << "❌ DEBUG ProjectTree::rowCount: ChildrenSize() threw exception";
                        count = 0;
                    }
                } else {
                    qDebug() << "🔍 DEBUG ProjectTree::rowCount: Project invalid (null pointer)";
                    count = 0;
                }
            } else {
                qDebug() << "🔍 DEBUG ProjectTree::rowCount: Index out of range:" << p.row() << "/" << projectList.size();
                count = 0;
            }
        } else {
            qDebug() << "🔍 DEBUG ProjectTree::rowCount: Unknown UUID format, returning 0";
            count = 0;
        }
    }
    
    qDebug() << "🔍 DEBUG ProjectTree::rowCount: Returning count:" << count;
    return count;
}

// Claude Generated - ProjectManager Integration for data display
QVariant ProjectTree::data(const QModelIndex& index, int role) const
{
    qDebug() << "🔍 DEBUG ProjectTree::data: Called with row:" << index.row() << "column:" << index.column() << "role:" << role << "valid:" << index.isValid();
    
    QVariant data;
    if (!index.isValid()) {
        qDebug() << "❌ DEBUG ProjectTree::data: Invalid index, returning empty";
        return data;
    }

    qDebug() << "🔍 DEBUG ProjectTree::data: Getting unified project list";
    QVector<QWeakPointer<DataClass>> projectList = getUnifiedProjectList();
    qDebug() << "🔍 DEBUG ProjectTree::data: Project list size:" << projectList.size();
    
    // Claude Generated - CRITICAL FIX: Declare internalPtr once at the top for entire method
    void* internalPtr = index.internalPointer();
    qDebug() << "🔍 DEBUG ProjectTree::data: InternalPtr:" << internalPtr;
    
    if (role == Qt::DisplayRole) {
        if (index.column() == 0) {
            if (internalPtr != nullptr) // Model Element (child item)
            {
                // Claude Generated - CRITICAL FIX: Find parent project without calling parent() method
                int uuidIndex = reinterpret_cast<quintptr>(internalPtr) - 1;
                if (uuidIndex >= 0 && uuidIndex < m_uuids.size()) {
                    QString childUuid = m_uuids[uuidIndex];
                    QStringList uuidParts = childUuid.split("|");
                    if (uuidParts.size() >= 3) {
                        QString parentProjectUuid = uuidParts[0];
                        // Find parent project by UUID
                        for (int i = 0; i < projectList.size(); ++i) {
                            QSharedPointer<DataClass> project = projectList[i].toStrongRef();
                            if (project && project->UUID() == parentProjectUuid) {
                                if (index.row() < project->ChildrenSize()) {
                                    QPointer<DataClass> child = project->Children(index.row());
                                    AbstractModel* model = qobject_cast<AbstractModel*>(child.data());
                                    if (model) {
                                        data = model->Name();
                                    } else {
                                        data = QString("Model #%1").arg(index.row());
                                    }
                                }
                                break;
                            }
                        }
                    }
                }
            } else // DataClass Element (top-level item)
            {
                if (index.row() < projectList.size()) {
                    QSharedPointer<DataClass> project = projectList[index.row()].toStrongRef();
                    if (project) {
                        data = project->ProjectTitle();
                    }
                }
            }
        } else if (index.column() == 1 && internalPtr == nullptr) {
            if (index.row() < projectList.size()) {
                QSharedPointer<DataClass> project = projectList[index.row()].toStrongRef();
                if (project) {
                    data = project->ChildrenSize();
                }
            }
        } else if (index.column() == 1 && internalPtr != nullptr) {
            // qDebug() << index.row() << projectList[index.row()].data()->ProjectTitle();
        }
    } else if (role == Qt::FontRole) {
        QFont font("SanSerif", 12);
        font.setWeight(QFont::Light);
        if (index.row() == m_active_row && index.column() == 0 && internalPtr == nullptr) {
            // Claude Generated - Enhanced active project font styling
            font.setWeight(QFont::ExtraBold);
            font.setPointSize(13); // Slightly larger for better visibility
        } else if (internalPtr != nullptr) {
            font.setItalic(true);
        }

        return font;
    } else if (role == Qt::BackgroundRole) {
        // Claude Generated - Background highlighting for active projects
        if (index.row() == m_active_row && index.column() == 0 && internalPtr == nullptr) {
            // Light blue background for active project
            return QColor(220, 235, 255);
        } else if (index.row() == m_active_row && internalPtr == nullptr) {
            // Extend highlighting to model count column
            return QColor(230, 240, 255);
        }
    } else if (role == Qt::ForegroundRole) {
        // Claude Generated - Enhanced text color for active projects
        if (index.row() == m_active_row && index.column() == 0 && internalPtr == nullptr) {
            // Darker blue text for active project
            return QColor(0, 70, 140);
        }
    } else if (role == Qt::ToolTipRole) {
        // Claude Generated - Contextual tooltips for better UX guidance
        if (internalPtr == nullptr) {
            // Project-level tooltip
            if (index.row() == m_active_row) {
                return tr("Active Project - Single-click models to focus them directly");
            } else {
                return tr("Inactive Project - Double-click to switch and focus, or single-click to switch only");
            }
        } else {
            // Model-level tooltip - simplified without parent lookup to avoid crashes
            return tr("Model - Double-click to focus");
        }
    }

    return data;
}

// Claude Generated - ProjectManager Integration for index creation (CRASH FIX)
QModelIndex ProjectTree::index(int row, int column, const QModelIndex& parent) const
{
    qDebug() << "🔍 DEBUG ProjectTree::index: Called with row:" << row << "column:" << column << "parent valid:" << parent.isValid();
    
    QModelIndex index;
    
    qDebug() << "🔍 DEBUG ProjectTree::index: Checking hasIndex for row:" << row << "column:" << column << "parent valid:" << parent.isValid();
    
    if (!hasIndex(row, column, parent)) {
        qDebug() << "❌ DEBUG ProjectTree::index: hasIndex() returned false, returning invalid index";
        qDebug() << "🔍 DEBUG ProjectTree::index: rowCount for parent:" << rowCount(parent) << "columnCount:" << columnCount(parent);
        return QModelIndex();
    }
    
    qDebug() << "✅ DEBUG ProjectTree::index: hasIndex() returned true, proceeding with index creation";

    QVector<QWeakPointer<DataClass>> projectList = getUnifiedProjectList();
    qDebug() << "🔍 DEBUG ProjectTree::index: ProjectList size:" << projectList.size();

    if (!parent.isValid()) {
        // Top-level project
        qDebug() << "🔍 DEBUG ProjectTree::index: Creating top-level index";
        
        if (row == -1) {
            qDebug() << "🔍 DEBUG ProjectTree::index: Invalid row -1";
            return index;
        }

        if (row < projectList.size()) {
            QSharedPointer<DataClass> project = projectList[row].toStrongRef();
            if (project) {
                QString uuid = project->UUID();
                qDebug() << "🔍 DEBUG ProjectTree::index: Project UUID:" << uuid;
                
                int uuidIndex = m_uuids.indexOf(uuid);
                if (uuidIndex == -1) {
                    qDebug() << "🔍 DEBUG ProjectTree::index: UUID not found in m_uuids, updating structure and retrying";
                    // Ensure structure is up to date - this handles timing issues where index() is called before UpdateStructure()
                    const_cast<ProjectTree*>(this)->UpdateStructure();
                    uuidIndex = m_uuids.indexOf(uuid);
                    if (uuidIndex == -1) {
                        qDebug() << "❌ DEBUG ProjectTree::index: UUID still not found after structure update, returning invalid index";
                        return index;
                    }
                }
                
                // Claude Generated - CRITICAL FIX: Top-level items use nullptr as internal pointer
                index = createIndex(row, column, nullptr);
                qDebug() << "🔍 DEBUG ProjectTree::index: Created top-level index successfully";
            } else {
                qDebug() << "🔍 DEBUG ProjectTree::index: Project is null pointer";
            }
        } else {
            qDebug() << "🔍 DEBUG ProjectTree::index: Row out of bounds:" << row << "/" << projectList.size();
        }
    } else {
        // Child model
        qDebug() << "🔍 DEBUG ProjectTree::index: Creating child index for parent row:" << parent.row();
        qDebug() << "🔍 DEBUG ProjectTree::index: Child row:" << row << "column:" << column;
        
        qDebug() << "🔍 DEBUG ProjectTree::index: Parent row:" << parent.row() << "projectList size:" << projectList.size();
        
        if (parent.row() < projectList.size()) {
            QSharedPointer<DataClass> parentProject = projectList[parent.row()].toStrongRef();
            qDebug() << "🔍 DEBUG ProjectTree::index: Parent project valid:" << (bool)parentProject;
            
            if (parentProject) {
                qDebug() << "🔍 DEBUG ProjectTree::index: Parent has" << parentProject->ChildrenSize() << "children, requesting child" << row;
            }
            
            if (parentProject && row < parentProject->ChildrenSize()) {
                qDebug() << "🔍 DEBUG ProjectTree::index: Parent project valid, accessing child" << row;
                
                // CRITICAL FIX: Safe model access
                QPointer<DataClass> child = parentProject->Children(row);
                if (!child) {
                    qDebug() << "❌ DEBUG ProjectTree::index: Child is null pointer";
                    return index;
                }
                
                AbstractModel* model = qobject_cast<AbstractModel*>(child.data());
                if (!model) {
                    qDebug() << "❌ DEBUG ProjectTree::index: Child is not an AbstractModel";
                    return index;
                }
                
                QString uuid = parentProject->UUID();
                // Claude Generated - Calculate child index using EXACT same logic as UpdateStructure
                QString modelPointer = QString::number(reinterpret_cast<quintptr>(model), 16);
                
                // Use exact same hash-based occurrence counting as UpdateStructure
                QHash<QString, int> pointerOccurrenceCount;
                int childIndex = -1; // Will be set when we find our target row
                
                for (int i = 0; i <= row; ++i) { // Include current row to mimic UpdateStructure iteration
                    QPointer<DataClass> child_i = parentProject->Children(i);
                    if (child_i) {
                        AbstractModel* model_i = qobject_cast<AbstractModel*>(child_i.data());
                        if (model_i) {
                            QString pointer_i = QString::number(reinterpret_cast<quintptr>(model_i), 16);
                            int currentChildIndex = pointerOccurrenceCount.value(pointer_i, 0);
                            pointerOccurrenceCount[pointer_i] = currentChildIndex + 1;
                            
                            // When we reach our target row, store the childIndex
                            if (i == row && pointer_i == modelPointer) {
                                childIndex = currentChildIndex;
                                break;
                            }
                        }
                    }
                }
                
                if (childIndex == -1) {
                    qDebug() << "❌ DEBUG ProjectTree::index: Failed to calculate childIndex for row" << row;
                    return index;
                }
                
                QString sub_uuid = uuid + "|" + QString::number(childIndex) + "|" + modelPointer;
                
                qDebug() << "🔍 DEBUG ProjectTree::index: Child UUID:" << sub_uuid;
                
                int subUuidIndex = m_uuids.indexOf(sub_uuid);
                if (subUuidIndex == -1) {
                    qDebug() << "🔍 DEBUG ProjectTree::index: Sub-UUID not found in m_uuids, updating structure and retrying";
                    // Ensure structure is up to date - this handles timing issues where index() is called before UpdateStructure()
                    const_cast<ProjectTree*>(this)->UpdateStructure();
                    subUuidIndex = m_uuids.indexOf(sub_uuid);
                    if (subUuidIndex == -1) {
                        qDebug() << "❌ DEBUG ProjectTree::index: Sub-UUID still not found after structure update:" << sub_uuid;
                        return index;
                    }
                }
                
                // Claude Generated - CRITICAL FIX: Child items use index+1 as internal pointer
                index = createIndex(row, column, reinterpret_cast<void*>(subUuidIndex + 1));
                qDebug() << "🔍 DEBUG ProjectTree::index: Created child index successfully";
            } else {
                if (!parentProject) {
                    qDebug() << "❌ DEBUG ProjectTree::index: Parent project is null";
                } else {
                    qDebug() << "❌ DEBUG ProjectTree::index: Child index out of bounds:" << row << "/" << parentProject->ChildrenSize();
                }
            }
        } else {
            qDebug() << "❌ DEBUG ProjectTree::index: Parent row out of bounds:" << parent.row() << "/" << projectList.size();
        }
    }

    qDebug() << "🔍 DEBUG ProjectTree::index: Returning index valid:" << index.isValid();
    return index;
}

// Claude Generated - ProjectManager Integration for parent finding
QModelIndex ProjectTree::parent(const QModelIndex& child) const
{
    qDebug() << "🔍 DEBUG ProjectTree::parent: Called with child valid:" << child.isValid();
    
    QModelIndex index;

    if (!child.isValid()) {
        qDebug() << "❌ DEBUG ProjectTree::parent: Child invalid, returning empty index";
        return index;
    }

    // Claude Generated - CRITICAL FIX: Use internalPointer directly, avoid UUID() method
    void* internalPtr = child.internalPointer();
    qDebug() << "🔍 DEBUG ProjectTree::parent: Child internalPtr:" << internalPtr;
    
    if (internalPtr == nullptr) {
        // Child is a top-level project - has no parent
        qDebug() << "🔍 DEBUG ProjectTree::parent: Child is top-level project, no parent";
        return index;
    }
    
    // Child is a model - its parent is the project at the child's row in parent()
    // For now, we need to find which project this child belongs to
    // We'll use a simpler approach: get UUID index and find the parent project
    int uuidIndex = reinterpret_cast<quintptr>(internalPtr) - 1;
    qDebug() << "🔍 DEBUG ProjectTree::parent: Child UUID index:" << uuidIndex;
    
    if (uuidIndex >= 0 && uuidIndex < m_uuids.size()) {
        QString childUuid = m_uuids[uuidIndex];
        qDebug() << "🔍 DEBUG ProjectTree::parent: Child UUID:" << childUuid;
        
        QStringList uuidParts = childUuid.split("|");
        if (uuidParts.size() >= 3) {
            QString parentProjectUuid = uuidParts[0];
            qDebug() << "🔍 DEBUG ProjectTree::parent: Parent project UUID:" << parentProjectUuid;
            
            // Find parent project by UUID
            QVector<QWeakPointer<DataClass>> projectList = getUnifiedProjectList();
            for (int i = 0; i < projectList.size(); ++i) {
                QSharedPointer<DataClass> project = projectList[i].toStrongRef();
                if (project && project->UUID() == parentProjectUuid) {
                    qDebug() << "🔍 DEBUG ProjectTree::parent: Found parent project at row" << i;
                    // Parent is always a top-level project (internalPointer = nullptr)
                    index = createIndex(i, 0, nullptr);
                    break;
                }
            }
        }
    }
    
    qDebug() << "🔍 DEBUG ProjectTree::parent: Returning parent index valid:" << index.isValid();

    return index;
}

QMimeData* ProjectTree::mimeData(const QModelIndexList& indexes) const
{
    ModelMime* mimeData = new ModelMime();

    QString data;
    QJsonObject object;
    for (const QModelIndex& index : qAsConst(indexes)) {
        if (index.isValid()) {
            QJsonObject d, top;
            if (parent(index).isValid()) {
                data = "Model: " + QString::number(parent(index).row()) + "-" + QString::number(index.row());
                mimeData->setModel(true);
                // Claude Generated - Updated for simplified UUID structure: project_uuid|child_index|pointer_hex
                QStringList uuids = UUID(index).split("|");
                if (uuids.size() >= 3) {
                    mimeData->setDataUUID(uuids[0]);
                    mimeData->setModelUUID(uuids[1]); // Now contains child index instead of ModelUUID
                    QString modelPointer = uuids[2];
                    int childIndex = uuids[1].toInt();
                    void* targetPointer = reinterpret_cast<void*>(modelPointer.toULongLong(nullptr, 16));
                    for (int i = 0; i < m_data_list->size(); ++i) {
                        if (mimeData->DataUUID() == (*m_data_list)[i].toStrongRef().data()->UUID()) {
                            if (childIndex >= 0 && childIndex < (*m_data_list)[i].toStrongRef().data()->ChildrenSize()) {
                                AbstractModel* model = qobject_cast<AbstractModel*>((*m_data_list)[i].toStrongRef().data()->Children(childIndex));
                                if (model && reinterpret_cast<void*>(model) == targetPointer) {
                                    // Claude Generated - Match specific model instance by index and pointer
                                    top = model->ExportModel(true, false);
                                    mimeData->setSupportSeries(model->SupportSeries());
                                }
                            }
                        }
                    }
                }
            } else {
                data = "Data: " + QString::number(index.row());
                mimeData->setDataUUID(UUID(index));
                for (int i = 0; i < m_data_list->size(); ++i) {
                    if (mimeData->DataUUID() == (*m_data_list)[i].toStrongRef().data()->UUID()) {
                        d = (*m_data_list)[i].toStrongRef().data()->ExportData();
                        top = (*m_data_list)[i].toStrongRef().data()->ExportChildren(true, false);
                    }
                }
                top["data"] = d;
            }
            mimeData->setModelIndex(index);
            QJsonDocument document = QJsonDocument(top);
            mimeData->setData("application/x-suprafitmodel", document.toJson());
        }
    }

    mimeData->setInstance(m_instance);
    mimeData->setText(data);
    return mimeData;
}

bool ProjectTree::canDropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& index) const
{
    Q_UNUSED(action)

    QString string = data->text();

    if (string.contains("file:///")) {
        return true;
    }

    if (!data->urls().isEmpty())
        return true;

    if (!qobject_cast<const ModelMime*>(data)) {
        /* This could be from suprafit but a different main instance */

        QByteArray sprmodel = data->data("application/x-suprafitmodel");
        QJsonDocument doc = QJsonDocument::fromJson(sprmodel);
        QJsonObject mod = doc.object();
        if (mod["model"].toInt() == 0) {
            if (!doc.isEmpty() && (!string.contains("Model") || !string.contains("Data"))) {
                return true;
            }
            return false;
        } else {
            if (index.isValid() && parent(index).isValid()) {
                return true;
            }
        }
        return false;
    }

    const ModelMime* d = qobject_cast<const ModelMime*>(data);

    QByteArray sprmodel = data->data("application/x-suprafitmodel");
    QJsonDocument doc = QJsonDocument::fromJson(sprmodel);

    if (d->Instance() != m_instance) {
        if (!doc.isEmpty() && string.contains("Data")) {
            return true;
        }
    }

    if (string == "SupraFitSimulation") {
        return true;
    }

    if (string.isEmpty() || string.isNull())
        return false;

    if (row == -1 && column == -1 && !index.isValid()) {
        const ModelMime* d = qobject_cast<const ModelMime*>(data);

        if (d->Index().parent().row() == -1)
            return false;

        if (d->Index().parent().row() < (*m_data_list).size() && d->Index().parent().row() >= 0) {
            if (!qApp->instance()->property("MetaSeries").toBool()) {
                if (d->SupportSeries()) {
                    return false;
                }
            }
            if ((*m_data_list)[d->Index().parent().row()].toStrongRef().data()->SFModel() == SupraFit::MetaModel)
                return false;
        }
        return true;
    }
    //  qDebug() << "MetaModel" << d->Index().row() << d->Index().parent().row() << row << column <<(*m_data_list)[d->Index().parent().row()].toStrongRef().data()->ProjectTitle();
    //    qDebug() << row << column << index.isValid() << d->Index().parent().row();

    if (index.isValid() && !parent(index).isValid()) {
        int r = index.row();
        const ModelMime* d = qobject_cast<const ModelMime*>(data);

        qDebug() << row << column;
        qDebug() << index.row() << index.column();
        
        // Claude Generated - CRITICAL FIX: Add bounds checking to prevent crash
        // Ensure r is valid and within m_data_list bounds before accessing
        if (r >= 0 && r < m_data_list->size() && 
            qobject_cast<MetaModel*>((*m_data_list)[r].toStrongRef().data()) && index.isValid()) {

            if (index.row() < m_data_list->size()) {
                if ((*m_data_list)[index.row()].toStrongRef().data()->SFModel() == SupraFit::MetaModel)
                    qDebug() << "can add metamodel";
                return true;
            }
            if (d->Index().parent().row() < (*m_data_list).size() && d->Index().parent().row() >= 0) {

                if ((*m_data_list)[d->Index().parent().row()].toStrongRef().data()->SFModel() == SupraFit::MetaModel) {
                    return false;
                }
                if (!qApp->instance()->property("MetaSeries").toBool()) {
                    if (d->SupportSeries()) {
                        return false;
                    }
                } else
                    return false;
            }
            if (d->Index().parent().isValid())
                return true;
            else
                return false; //emit UiMessage(tr("It doesn't make sense to add whole project to a meta model.\nTry one of the models within this project."));
        } else if (index.isValid()) {
            if (!d->Index().parent().isValid()) {
                return true; //emit CopySystemParameter(d->Index(), r);
            } else
                return false; //emit UiMessage(tr("Nothing to tell"));
        }
        return true;
    } else if (index.isValid() && parent(index).isValid()) {
        return true;
    } else
        return true;
    return true;
}

bool ProjectTree::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& index)
{
    Q_UNUSED(action)

    QString string = data->text();

    if (string.contains("file:///")) {
        QStringList list = string.split("\n");
        for (QString str : list) {
            if (!str.isEmpty() && !str.isNull())
#ifdef _WIN32
                emit LoadFile(str.remove("file:///"), 0);
#else
                emit LoadFile(str.remove("file://"), 0);
#endif
        }
        return true;
    }

    if (!data->urls().isEmpty()) {
        for (const QUrl& url : data->urls()) {
            emit LoadFile(url.toLocalFile(), 0);
        }
        return true;
    }
    if (!qobject_cast<const ModelMime*>(data)) {
        /* This could be from suprafit but a different main instance */

        QByteArray sprmodel = data->data("application/x-suprafitmodel");
        QJsonDocument doc = QJsonDocument::fromJson(sprmodel);
        QJsonObject mod = doc.object();
        if (mod["model"].toInt() == 0) {
            if (!doc.isEmpty() && (!string.contains("Model") || !string.contains("Data"))) {
                emit LoadJsonObject(mod);
                return true;
            }
            return false;
        } else {
            qDebug() << row << index.row() << parent(index).row();
            if (index.isValid() && parent(index).isValid()) {
                emit CopyModel(mod, parent(index).row(), index.row());
                return true;
            }
        }
        return false;
    }

    const ModelMime* d = qobject_cast<const ModelMime*>(data);

    QByteArray sprmodel = data->data("application/x-suprafitmodel");
    QJsonDocument doc = QJsonDocument::fromJson(sprmodel);

    if (d->Instance() != m_instance) {
        if (!doc.isEmpty() && string.contains("Data")) {
            emit LoadJsonObject(doc.object());
            return true;
        }
    }

    if (string == "SupraFitSimulation") {
        emit LoadJsonObject(doc.object());
        return true;
    }

    if (string.isEmpty() || string.isNull())
        return false;

    if (row == -1 && column == -1 && !index.isValid()) {
        const ModelMime* d = qobject_cast<const ModelMime*>(data);
        if ((*m_data_list)[d->Index().parent().row()].toStrongRef().data()->SFModel() == SupraFit::MetaModel)
            return false;
        emit AddMetaModel(d->Index(), -1);
        return true;
    }
    if (index.isValid() && !parent(index).isValid()) {
        int r = index.row();
        const ModelMime* d = qobject_cast<const ModelMime*>(data);

        if (qobject_cast<MetaModel*>((*m_data_list)[r].toStrongRef().data()) && index.isValid()) {
            if ((*m_data_list)[d->Index().parent().row()].toStrongRef().data()->SFModel() == SupraFit::MetaModel)
                return false;

            if (d->Index().parent().isValid())
                emit AddMetaModel(d->Index(), r);
            else
                emit UiMessage(tr("It doesn't make sense to add whole project to a meta model.\nTry one of the models within this project."));
        } else if (index.isValid()) {
            if (!d->Index().parent().isValid())
                emit CopySystemParameter(d->Index(), r);
            else
                emit UiMessage(tr("Nothing to tell"));
        }
        return true;
    } else if (index.isValid() && parent(index).isValid()) {
        const ModelMime* d = qobject_cast<const ModelMime*>(data);
        QByteArray sprmodel = d->data("application/x-suprafitmodel");
        QJsonDocument doc = QJsonDocument::fromJson(sprmodel);
        QJsonObject mod = doc.object();
        emit CopyModel(mod, parent(index).row(), index.row());
    } else
        return true;
    return true;
}

void ProjectTree::setActiveIndex(int index)
{
    m_active_row = index;
    layoutChanged();
}
