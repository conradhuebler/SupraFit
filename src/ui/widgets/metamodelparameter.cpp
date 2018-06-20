/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/core/models.h"

#include <QtCore/QAbstractItemModel>
#include <QtCore/QFile>
#include <QtCore/QSharedPointer>

#include <QtWidgets/QGridLayout>
#include <QtWidgets/QTreeView>

#include "metamodelparameter.h"

int ParameterTree::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)
    return 1;
}

int ParameterTree::rowCount(const QModelIndex& parent) const
{
    int count = m_model.data()->GlobalParameterSize() + m_model.data()->LocalParameterSize();

    if (parent.isValid()) {
        int* pos = static_cast<int*>(parent.internalPointer());
        if (pos == two)
            count = 0;
        else
            count = (m_model.data()->CombinedGlobal() << m_model.data()->CombinedLocal())[parent.row()].second.size();
        qDebug() << *pos << parent.row();
        //if(parent.row() < m_model.data()->GlobalParameterSize())
        //    qDebug() << (m_model.data()->CombinedGlobal() << m_model.data()->CombinedLocal())[parent.row()];
    }
    return count;
}

QVariant ParameterTree::data(const QModelIndex& index, int role) const
{
    QVariant data;
    if (!index.isValid())
        return data;

    if (role == Qt::DisplayRole) {
        int* pos = static_cast<int*>(index.internalPointer());

        if (*pos == 0)
            data = (m_model.data()->CombinedGlobal() << m_model.data()->CombinedLocal())[index.row()].first; //m_model->GlobalParameter(index.row());
        else {
            data = QString("source"); //Model2Name(qobject_cast<AbstractModel*>(dataclass)->SFModel());
        }
    }
    return data;
}

QModelIndex ParameterTree::index(int row, int column, const QModelIndex& parent) const
{

    QModelIndex index;
    if (!hasIndex(row, column, parent))
        index = QModelIndex();

    if (!parent.isValid()) {
        index = createIndex(row, column, zero);
    } else {
        if (static_cast<int*>(index.internalPointer()) == one)
            index = createIndex(row, column, two);
        else
            index = createIndex(row, column, one);
    }

    return index;
}

QModelIndex ParameterTree::parent(const QModelIndex& child) const
{
    QModelIndex index;

    if (!child.isValid())
        return index;

    /*
    bool model = qobject_cast<AbstractModel*>(data);

    QPointer<DataClass> p;
    int dataclass = -1, modelclass = -1;
    int count1 = 0, count2 = 0;
    for (int i = 0; i < m_project_list->size(); ++i) {
        for (int j = 0; j < (*m_project_list)[i]->ModelCount(); ++j) {

            if ((*m_project_list)[i]->Data() == data) {
                dataclass = i;
                count1++;
            }
            if (!model)
                continue;

            if ((*m_project_list)[i]->Model(j) == qobject_cast<AbstractModel*>(data)) {
                modelclass = j;
                dataclass = i;
                p = (*m_project_list)[i]->Data();
                count2++;
                break;
            }
        }
    }
    if (modelclass != -1)
        index = createIndex(dataclass, 0, p);
    */
    return index;
}

MetaModelParameter::MetaModelParameter(QSharedPointer<AbstractModel> model)
{
    m_model = qSharedPointerCast<MetaModel>(model);
    setUi();
}

void MetaModelParameter::setUi()
{
    m_layout = new QGridLayout;

    m_tree = new QTreeView;
    m_treemodel = new ParameterTree(m_model);
    m_tree->setModel(m_treemodel);

    m_layout->addWidget(m_tree, 0, 0);

    setLayout(m_layout);

    connect(m_model.data(), &MetaModel::ModelAdded, this, [this]() {
        m_treemodel->layoutChanged();
    });
    connect(m_model.data(), &MetaModel::Recalculated, this, [this]() {
        m_treemodel->layoutChanged();
    });
}
