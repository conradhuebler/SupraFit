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

QSize ProjectTreeEntry::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    if (index.parent().isValid())
        return QSize(50, 25);
    else
        return QSize(100, 25);
}

void ProjectTree::UpdateStructure()
{
    for (int i = 0; i < m_data_list->size(); ++i) {
        QString uuid = (*m_data_list)[i].toStrongRef().data()->UUID();
        // qDebug() << (*m_data_list)[i].toStrongRef().data()->UUID() << (*m_data_list)[i].toStrongRef().data()->SFModel() << (*m_data_list)[i].toStrongRef().data()->ProjectTitle();

        if (!m_uuids.contains(uuid)) {
            m_uuids << uuid;
            m_ptr_uuids << &m_uuids.last();
        }

        for (int j = 0; j < (*m_data_list)[i].toStrongRef().data()->ChildrenSize(); ++j) {
            QString sub_uuid = uuid + "|" + qobject_cast<AbstractModel*>((*m_data_list)[i].toStrongRef().data()->Children(j))->ModelUUID();

            if (!m_uuids.contains(sub_uuid)) {
                m_uuids << sub_uuid;
                m_ptr_uuids << &m_uuids.last();
            }
        }
    }
    layoutChanged();
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

int ProjectTree::rowCount(const QModelIndex& p) const
{
    int count = m_data_list->size();
    if (p.isValid()) {

        QString uuid = UUID(p);
        if (uuid.size() == 77) // Model Element
        {
            count = 0;

        } else if (uuid.size() == 38) // DataClass Element
        {
            count = (*m_data_list)[p.row()].toStrongRef().data()->ChildrenSize();
        } else
            count = 0;
    }
    return count;
}

QVariant ProjectTree::data(const QModelIndex& index, int role) const
{
    QVariant data;
    if (!index.isValid())
        return data;

    if (role == Qt::DisplayRole) {
        if (index.column() == 0) {
            if (parent(index).isValid()) // Model Element
            {
                data = qobject_cast<AbstractModel*>((*m_data_list)[parent(index).row()].toStrongRef().data()->Children(index.row()))->Name();

            } else // DataClass Element
            {
                data = (*m_data_list)[index.row()].toStrongRef().data()->ProjectTitle();
            }
        } else if (index.column() == 1 && !parent(index).isValid()) {
            data = (*m_data_list)[index.row()].toStrongRef().data()->ChildrenSize();
        } else if (index.column() == 1 && parent(index).isValid()) {
            // qDebug() << index.row() << (*m_data_list)[index.row()].data()->ProjectTitle();
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

QModelIndex ProjectTree::index(int row, int column, const QModelIndex& parent) const
{
    QModelIndex index;
    if (!hasIndex(row, column, parent))
        index = QModelIndex();

    if (!parent.isValid()) {
        if (row == -1) {
            return index;
        }

        if (row < m_data_list->size()) {
            QString uuid = (*m_data_list)[row].toStrongRef().data()->UUID();
            if (m_uuids.indexOf(uuid) == -1)
                return index;
            index = createIndex(row, column, m_ptr_uuids[m_uuids.indexOf(uuid)]);
        }
    } else {
        if (parent.row() < m_data_list->size()) {
            if (row < (*m_data_list)[parent.row()].toStrongRef().data()->ChildrenSize()) {
                QString uuid = (*m_data_list)[parent.row()].toStrongRef().data()->UUID();
                QString sub_uuid = uuid + "|" + qobject_cast<AbstractModel*>((*m_data_list)[parent.row()].toStrongRef().data()->Children(row))->ModelUUID();
                index = createIndex(row, column, m_ptr_uuids[m_uuids.indexOf(sub_uuid)]);
            }
        }
    }

    return index;
}

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
        for (int i = 0; i < m_data_list->size(); ++i) {
            if ((*m_data_list)[i].toStrongRef().data()->UUID() == uuids[0]) {
                for (int j = 0; j < (*m_data_list)[i].toStrongRef().data()->ChildrenSize(); ++j) {
                    if (qobject_cast<AbstractModel*>((*m_data_list)[i].toStrongRef().data()->Children(j))->ModelUUID() == uuids[1]) {
                        index = createIndex(i, 0, m_ptr_uuids[m_uuids.indexOf(uuids[0])]);
                    }
                }
            }
        }
    } else
        return index;

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
                emit LoadFile(str.remove("file:///"));
#else
                emit LoadFile(str.remove("file://"));
#endif
        }
        return true;
    }

    if (!data->urls().isEmpty()) {
        for (const QUrl& url : data->urls()) {
            emit LoadFile(url.toLocalFile());
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
