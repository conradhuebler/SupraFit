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
#include "src/core/models/meta_model.h" // MetaModel class (was transitive via models.h)

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
    // Clear existing UUID mappings
    m_uuids.clear();
    m_ptr_uuids.clear();

    QVector<QWeakPointer<DataClass>> projectList = getUnifiedProjectList();

    for (int i = 0; i < projectList.size(); ++i) {
        QSharedPointer<DataClass> project = projectList[i].toStrongRef();
        if (!project) {
#ifdef DEBUG_ON
            qDebug() << "ProjectTree::UpdateStructure: Project" << i << "is null, skipping";
#endif
            continue;
        }

        QString uuid = project->UUID();

        // Add project UUID - Claude Generated - CRITICAL FIX: Top-level items need nullptr as internal pointer
        if (!m_uuids.contains(uuid)) {
            m_uuids << uuid;
            // Top-level projects use nullptr as internal pointer (Qt convention)
            m_ptr_uuids << nullptr;
        }

        // CRITICAL FIX: Safe model processing with pointer-based child index counting
        int childrenCount = project->ChildrenSize();

        // Claude Generated - Track pointer occurrence counts for proper child indexing
        QHash<QString, int> pointerOccurrenceCount;

        for (int j = 0; j < childrenCount; ++j) {
            QPointer<DataClass> child = project->Children(j);
            if (!child) {
#ifdef DEBUG_ON
                qDebug() << "ProjectTree::UpdateStructure: Child" << j << "is null pointer, skipping";
#endif
                continue;
            }
            
            AbstractModel* model = qobject_cast<AbstractModel*>(child.data());
            if (!model) {
#ifdef DEBUG_ON
                qDebug() << "ProjectTree::UpdateStructure: Child" << j << "is not an AbstractModel, skipping";
#endif
                continue;
            }
            
            // Claude Generated - Calculate child index as pointer occurrence count (not absolute position)
            QString modelPointer = QString::number(reinterpret_cast<quintptr>(model), 16);
            int childIndex = pointerOccurrenceCount.value(modelPointer, 0);
            pointerOccurrenceCount[modelPointer] = childIndex + 1;

            QString sub_uuid = uuid + "|" + QString::number(childIndex) + "|" + modelPointer;

            // Claude Generated - Force addition for child index differentiation, even if pointer matches
            // This ensures that duplicate model instances with different child indices get unique UUIDs
            if (!m_uuids.contains(sub_uuid)) {
                m_uuids << sub_uuid;
                // Child items use index+1 as internal pointer (0 is reserved for top-level items)
                m_ptr_uuids << reinterpret_cast<void*>(m_uuids.size());
            } else {
                // Claude Generated - Force unique UUID when duplicate pointer detected
                QString forced_uuid = uuid + "|" + QString::number(childIndex) + "|" + modelPointer + "|forced";
                m_uuids << forced_uuid;
                m_ptr_uuids << reinterpret_cast<void*>(m_uuids.size());
            }
        }
    }

    // Claude Generated - Safe layoutChanged() call with proper model notifications
    try {
        beginResetModel();
        endResetModel();
    } catch (...) {
#ifdef DEBUG_ON
        qDebug() << "ProjectTree::UpdateStructure: Layout reset failed, continuing";
#endif
    }
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
#ifdef DEBUG_ON
        qDebug() << "ProjectTree::UUID: Invalid UUID index" << uuidIndex << "/ valid range: 0-" << (m_uuids.size() - 1);
#endif
        return QString();
    }
    
    return m_uuids[uuidIndex];
}

int ProjectTree::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        void* internalPtr = parent.internalPointer();

        if (internalPtr == nullptr) {
            // Parent is a top-level project, children (models) have 1 column
            return 1;
        } else {
            // Parent is a model, models don't have children
            return 0;
        }
    } else {
        return 2;
    }
}

// Claude Generated - ProjectManager Integration for rowCount
int ProjectTree::rowCount(const QModelIndex& p) const
{
    QVector<QWeakPointer<DataClass>> projectList = getUnifiedProjectList();
    int count = projectList.size();

    if (p.isValid()) {
        // Claude Generated - CRITICAL FIX: Safe UUID access with proper error handling
        QString uuid;
        void* internalPtr = p.internalPointer();

        if (!internalPtr) {
            // Top-level item: get UUID directly from project
            if (p.row() >= 0 && p.row() < projectList.size()) {
                QSharedPointer<DataClass> project = projectList[p.row()].toStrongRef();
                if (project) {
                    uuid = project->UUID();
                } else {
#ifdef DEBUG_ON
                    qDebug() << "ProjectTree::rowCount: Project pointer is null";
#endif
                    return 0;
                }
            } else {
#ifdef DEBUG_ON
                qDebug() << "ProjectTree::rowCount: Row out of bounds:" << p.row() << "/" << projectList.size();
#endif
                return 0;
            }
        } else {
            // Child item: get UUID from m_uuids array
            int uuidIndex = reinterpret_cast<quintptr>(internalPtr) - 1;
            if (uuidIndex >= 0 && uuidIndex < m_uuids.size()) {
                uuid = m_uuids[uuidIndex];
            } else {
#ifdef DEBUG_ON
                qDebug() << "ProjectTree::rowCount: UUID index out of bounds:" << uuidIndex << "/" << m_uuids.size();
#endif
                return 0;
            }
        }

        if (uuid.size() >= 50) // Model Element (simplified structure: project_uuid|child_index|pointer_hex)
        {
            count = 0;
        } else if (uuid.size() == 38) // DataClass Element
        {
            if (p.row() < projectList.size()) {
                QSharedPointer<DataClass> project = projectList[p.row()].toStrongRef();
                if (project) {
                    try {
                        count = project->ChildrenSize();
                    } catch (...) {
#ifdef DEBUG_ON
                        qDebug() << "ProjectTree::rowCount: ChildrenSize() threw exception";
#endif
                        count = 0;
                    }
                } else {
                    count = 0;
                }
            } else {
                count = 0;
            }
        } else {
            count = 0;
        }
    }

    return count;
}

// Claude Generated - ProjectManager Integration for data display
QVariant ProjectTree::data(const QModelIndex& index, int role) const
{
    QVariant data;
    if (!index.isValid()) {
        return data;
    }

    QVector<QWeakPointer<DataClass>> projectList = getUnifiedProjectList();

    // Claude Generated - CRITICAL FIX: Declare internalPtr once at the top for entire method
    void* internalPtr = index.internalPointer();

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
            // Model children don't have data in column 1
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
    QModelIndex index;

    if (!hasIndex(row, column, parent)) {
        return QModelIndex();
    }

    QVector<QWeakPointer<DataClass>> projectList = getUnifiedProjectList();

    if (!parent.isValid()) {
        // Top-level project
        if (row == -1) {
            return index;
        }

        if (row < projectList.size()) {
            QSharedPointer<DataClass> project = projectList[row].toStrongRef();
            if (project) {
                QString uuid = project->UUID();

                int uuidIndex = m_uuids.indexOf(uuid);
                if (uuidIndex == -1) {
                    // Ensure structure is up to date - this handles timing issues where index() is called before UpdateStructure()
                    const_cast<ProjectTree*>(this)->UpdateStructure();
                    uuidIndex = m_uuids.indexOf(uuid);
                    if (uuidIndex == -1) {
#ifdef DEBUG_ON
                        qDebug() << "ProjectTree::index: UUID still not found after structure update, returning invalid index";
#endif
                        return index;
                    }
                }

                // Claude Generated - CRITICAL FIX: Top-level items use nullptr as internal pointer
                index = createIndex(row, column, nullptr);
            }
        }
    } else {
        // Child model
        if (parent.row() < projectList.size()) {
            QSharedPointer<DataClass> parentProject = projectList[parent.row()].toStrongRef();

            if (parentProject && row < parentProject->ChildrenSize()) {
                // CRITICAL FIX: Safe model access
                QPointer<DataClass> child = parentProject->Children(row);
                if (!child) {
#ifdef DEBUG_ON
                    qDebug() << "ProjectTree::index: Child is null pointer";
#endif
                    return index;
                }
                
                AbstractModel* model = qobject_cast<AbstractModel*>(child.data());
                if (!model) {
#ifdef DEBUG_ON
                    qDebug() << "ProjectTree::index: Child is not an AbstractModel";
#endif
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
#ifdef DEBUG_ON
                    qDebug() << "ProjectTree::index: Failed to calculate childIndex for row" << row;
#endif
                    return index;
                }
                
                QString sub_uuid = uuid + "|" + QString::number(childIndex) + "|" + modelPointer;

                int subUuidIndex = m_uuids.indexOf(sub_uuid);
                if (subUuidIndex == -1) {
                    // Ensure structure is up to date - this handles timing issues where index() is called before UpdateStructure()
                    const_cast<ProjectTree*>(this)->UpdateStructure();
                    subUuidIndex = m_uuids.indexOf(sub_uuid);
                    if (subUuidIndex == -1) {
#ifdef DEBUG_ON
                        qDebug() << "ProjectTree::index: Sub-UUID still not found after structure update:" << sub_uuid;
#endif
                        return index;
                    }
                }

                // Claude Generated - CRITICAL FIX: Child items use index+1 as internal pointer
                index = createIndex(row, column, reinterpret_cast<void*>(subUuidIndex + 1));
            }
        }
    }

    return index;
}

// Claude Generated - ProjectManager Integration for parent finding
QModelIndex ProjectTree::parent(const QModelIndex& child) const
{
    QModelIndex index;

    if (!child.isValid()) {
        return index;
    }

    // Claude Generated - CRITICAL FIX: Use internalPointer directly, avoid UUID() method
    void* internalPtr = child.internalPointer();

    if (internalPtr == nullptr) {
        // Child is a top-level project - has no parent
        return index;
    }
    
    // Child is a model - its parent is the project at the child's row in parent()
    // For now, we need to find which project this child belongs to
    // We'll use a simpler approach: get UUID index and find the parent project
    int uuidIndex = reinterpret_cast<quintptr>(internalPtr) - 1;

    if (uuidIndex >= 0 && uuidIndex < m_uuids.size()) {
        QString childUuid = m_uuids[uuidIndex];

        QStringList uuidParts = childUuid.split("|");
        if (uuidParts.size() >= 3) {
            QString parentProjectUuid = uuidParts[0];

            // Find parent project by UUID
            QVector<QWeakPointer<DataClass>> projectList = getUnifiedProjectList();
            for (int i = 0; i < projectList.size(); ++i) {
                QSharedPointer<DataClass> project = projectList[i].toStrongRef();
                if (project && project->UUID() == parentProjectUuid) {
                    // Parent is always a top-level project (internalPointer = nullptr)
                    index = createIndex(i, 0, nullptr);
                    break;
                }
            }
        }
    }

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

    if (index.isValid() && !parent(index).isValid()) {
        int r = index.row();
        const ModelMime* d = qobject_cast<const ModelMime*>(data);

        // Claude Generated - CRITICAL FIX: Add bounds checking to prevent crash
        // Ensure r is valid and within m_data_list bounds before accessing
        if (r >= 0 && r < m_data_list->size() && 
            qobject_cast<MetaModel*>((*m_data_list)[r].toStrongRef().data()) && index.isValid()) {

            if (index.row() < m_data_list->size()) {
                if ((*m_data_list)[index.row()].toStrongRef().data()->SFModel() == SupraFit::MetaModel) {
                    // MetaModel detected - allow addition
                }
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
