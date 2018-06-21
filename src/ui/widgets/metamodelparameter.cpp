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

    int count = 0;
    int* pos = static_cast<int*>(parent.internalPointer());

    if (!parent.isValid())
        count = m_model.data()->GlobalParameterSize() + m_model.data()->LocalParameterSize();
    else if (pos == zero)
        count = (m_model.data()->CombinedGlobal() << m_model.data()->CombinedLocal())[parent.row()].second.size();
    else
        count = 0;

    return count;
}

QVariant ParameterTree::data(const QModelIndex& index, int role) const
{
    QVariant data;
    if (!index.isValid())
        return data;

    int* pos = static_cast<int*>(index.internalPointer());

    if (role == Qt::DisplayRole) {
        qDebug() << pos << *pos;
        if (pos == zero)
            data = (m_model.data()->CombinedGlobal() << m_model.data()->CombinedLocal())[index.row()].first;
        else if (pos == one)
            data = QString("one"); //Model2Name(qobject_cast<AbstractModel*>(dataclass)->SFModel());
        else
            data = QString("two");
    }
    return data;
}

QModelIndex ParameterTree::index(int row, int column, const QModelIndex& parent) const
{
    QModelIndex index;
    if (!hasIndex(row, column, parent))
        index = QModelIndex();

    int* pos = static_cast<int*>(parent.internalPointer());

    if (pos == NULL)
        index = createIndex(row, column, zero);
    else if (pos == zero)
        index = createIndex(row, column, one);
    else
        index = createIndex(row, column, two);

    return index;
}

QModelIndex ParameterTree::parent(const QModelIndex& child) const
{
    QModelIndex index;

    if (!child.isValid())
        return index;

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
