/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2020 - 2022 Conrad Hübler <Conrad.Huebler@gmx.net>
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
    #ifdef DEBUG_ON
    qDebug() << "ProjectTree::getUnifiedProjectList: Checking ProjectManager for projects";
    #endif
    SupraFit::ProjectManager& projectManager = SupraFit::ProjectManager::instance();
    #ifdef DEBUG_ON
    qDebug() << "ProjectTree::getUnifiedProjectList: ProjectManager has" << projectManager.getProjectCount() << "projects loaded";
    #endif
    QStringList managedProjectIds = projectManager.getLoadedProjectIds();
    #ifdef DEBUG_ON
    qDebug() << "ProjectTree::getUnifiedProjectList: Managed Project IDs:" << managedProjectIds;
    #endif
    if (!managedProjectIds.isEmpty()) {
        qDebug() << "ProjectTree::getUnifiedProjectList: Using ProjectManager data with" << managedProjectIds.size() << "projects";
        
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
        qDebug() << "ProjectTree::getUnifiedProjectList: Using legacy m_data_list with" << m_data_list->size() << "projects";
        return *m_data_list;
    }
    
    qDebug() << "ProjectTree::getUnifiedProjectList: No projects available from either source";
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

        // Add project UUID
        if (!m_uuids.contains(uuid)) {
            m_uuids << uuid;
            m_ptr_uuids << &m_uuids.last();
            qDebug() << "🔍 DEBUG ProjectTree::UpdateStructure: Added project UUID to lists";
        }

        // CRITICAL FIX: Safe model processing
        int childrenCount = project->ChildrenSize();
        qDebug() << "🔍 DEBUG ProjectTree::UpdateStructure: Project has" << childrenCount << "children";
        
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
            
            QString modelUuid = model->ModelUUID();
            QString sub_uuid = uuid + "|" + modelUuid;
            qDebug() << "🔍 DEBUG ProjectTree::UpdateStructure: Child" << j << "Model UUID:" << modelUuid << "Sub-UUID:" << sub_uuid;

            if (!m_uuids.contains(sub_uuid)) {
                m_uuids << sub_uuid;
                m_ptr_uuids << &m_uuids.last();
                qDebug() << "🔍 DEBUG ProjectTree::UpdateStructure: Added child UUID to lists";
            } else {
                qDebug() << "🔍 DEBUG ProjectTree::UpdateStructure: Sub-UUID already exists";
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
    int i = m_ptr_uuids.indexOf(index.internalPointer());
    if (i == -1)
        return QString();

    return m_uuids[i];
}

int ProjectTree::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 1;
    else
        return 2;
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
        
        // Safe UUID access with null checks
        QString uuid;
        try {
            uuid = UUID(p);
            qDebug() << "🔍 DEBUG ProjectTree::rowCount: UUID:" << uuid << "length:" << uuid.size();
        } catch (...) {
            qDebug() << "❌ DEBUG ProjectTree::rowCount: UUID() threw exception";
            return 0;
        }
        
        if (uuid.size() == 77) // Model Element
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
    QVariant data;
    if (!index.isValid())
        return data;

    QVector<QWeakPointer<DataClass>> projectList = getUnifiedProjectList();
    
    if (role == Qt::DisplayRole) {
        if (index.column() == 0) {
            if (parent(index).isValid()) // Model Element
            {
                QModelIndex parentIndex = parent(index);
                if (parentIndex.row() < projectList.size()) {
                    QSharedPointer<DataClass> project = projectList[parentIndex.row()].toStrongRef();
                    qDebug() << "🔍 DEBUG ProjectTree::data: Project" << parentIndex.row() << "valid:" << (bool)project;
                    if (project && index.row() < project->ChildrenSize()) {
                        qDebug() << "🔍 DEBUG ProjectTree::data: Accessing child" << index.row() << "/" << project->ChildrenSize();
                        QPointer<DataClass> child = project->Children(index.row());
                        qDebug() << "🔍 DEBUG ProjectTree::data: Child valid:" << (bool)child.data();
                        AbstractModel* model = qobject_cast<AbstractModel*>(child.data());
                        if (model) {
                            qDebug() << "🔍 DEBUG ProjectTree::data: Model cast successful:" << model->Name();
                            data = model->Name();
                        } else {
                            qDebug() << "🔍 DEBUG ProjectTree::data: Model cast failed, using fallback";
                            data = QString("Model #%1").arg(index.row());
                        }
                    } else {
                        qDebug() << "🔍 DEBUG ProjectTree::data: Invalid project or index out of bounds";
                    }
                } else {
                    qDebug() << "🔍 DEBUG ProjectTree::data: Parent index out of range:" << parentIndex.row() << "/" << projectList.size();
                }
            } else // DataClass Element
            {
                if (index.row() < projectList.size()) {
                    QSharedPointer<DataClass> project = projectList[index.row()].toStrongRef();
                    if (project) {
                        data = project->ProjectTitle();
                    }
                }
            }
        } else if (index.column() == 1 && !parent(index).isValid()) {
            if (index.row() < projectList.size()) {
                QSharedPointer<DataClass> project = projectList[index.row()].toStrongRef();
                if (project) {
                    data = project->ChildrenSize();
                }
            }
        } else if (index.column() == 1 && parent(index).isValid()) {
            // qDebug() << index.row() << projectList[index.row()].data()->ProjectTitle();
        }
    } else if (role == Qt::FontRole) {
        QFont font("SanSerif", 12);
        font.setWeight(QFont::Light);
        if (index.row() == m_active_row && index.column() == 0 && !parent(index).isValid()) {
            font.setWeight(QFont::ExtraBold);
            // font.setPointSize(14);
        } else if (parent(index).isValid()) {
            font.setItalic(true);
        }

        return font;
    }

    return data;
}

// Claude Generated - ProjectManager Integration for index creation (CRASH FIX)
QModelIndex ProjectTree::index(int row, int column, const QModelIndex& parent) const
{
    qDebug() << "🔍 DEBUG ProjectTree::index: Called with row:" << row << "column:" << column << "parent valid:" << parent.isValid();
    
    QModelIndex index;
    if (!hasIndex(row, column, parent)) {
        qDebug() << "🔍 DEBUG ProjectTree::index: hasIndex() returned false, returning invalid index";
        return QModelIndex();
    }

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
                    qDebug() << "🔍 DEBUG ProjectTree::index: UUID not found in m_uuids, returning invalid index";
                    return index;
                }
                
                if (uuidIndex >= m_ptr_uuids.size()) {
                    qDebug() << "❌ DEBUG ProjectTree::index: UUID index out of bounds in m_ptr_uuids:" << uuidIndex << "/" << m_ptr_uuids.size();
                    return index;
                }
                
                index = createIndex(row, column, m_ptr_uuids[uuidIndex]);
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
        
        if (parent.row() < projectList.size()) {
            QSharedPointer<DataClass> parentProject = projectList[parent.row()].toStrongRef();
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
                QString modelUuid = model->ModelUUID();
                QString sub_uuid = uuid + "|" + modelUuid;
                
                qDebug() << "🔍 DEBUG ProjectTree::index: Child UUID:" << sub_uuid;
                
                int subUuidIndex = m_uuids.indexOf(sub_uuid);
                if (subUuidIndex == -1) {
                    qDebug() << "❌ DEBUG ProjectTree::index: Sub-UUID not found in m_uuids:" << sub_uuid;
                    return index;
                }
                
                if (subUuidIndex >= m_ptr_uuids.size()) {
                    qDebug() << "❌ DEBUG ProjectTree::index: Sub-UUID index out of bounds:" << subUuidIndex << "/" << m_ptr_uuids.size();
                    return index;
                }
                
                index = createIndex(row, column, m_ptr_uuids[subUuidIndex]);
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
    QModelIndex index;

    if (!child.isValid())
        return index;

    QString uuid = UUID(child);

    if (!(uuid.size() == 77 || uuid.size() == 38))
        return index;

    QStringList uuids = uuid.split("|");
    if (uuids.size() == 2) {
        QVector<QWeakPointer<DataClass>> projectList = getUnifiedProjectList();
        
        for (int i = 0; i < projectList.size(); ++i) {
            QSharedPointer<DataClass> project = projectList[i].toStrongRef();
            if (project && project->UUID() == uuids[0]) {
                for (int j = 0; j < project->ChildrenSize(); ++j) {
                    if (qobject_cast<AbstractModel*>(project->Children(j))->ModelUUID() == uuids[1]) {
                        index = createIndex(i, 0, m_ptr_uuids[m_uuids.indexOf(uuids[0])]);
                        break;
                    }
                }
                break;
            }
        }
    } else {
        return index;
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
                QStringList uuids = UUID(index).split("|");
                if (uuids.size() == 2) {
                    mimeData->setDataUUID(uuids[0]);
                    mimeData->setModelUUID(uuids[1]);
                    for (int i = 0; i < m_data_list->size(); ++i) {
                        if (mimeData->DataUUID() == (*m_data_list)[i].toStrongRef().data()->UUID()) {
                            for (int j = 0; j < (*m_data_list)[i].toStrongRef().data()->ChildrenSize(); ++j) {
                                if (qobject_cast<AbstractModel*>((*m_data_list)[i].toStrongRef().data()->Children(j))->ModelUUID() == mimeData->ModelUUID()) {
                                    AbstractModel* model = qobject_cast<AbstractModel*>((*m_data_list)[i].toStrongRef().data()->Children(j));
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
        if (qobject_cast<MetaModel*>((*m_data_list)[r].toStrongRef().data()) && index.isValid()) {

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
