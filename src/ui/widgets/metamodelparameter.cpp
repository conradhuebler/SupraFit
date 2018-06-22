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
    /*
    if (!parent.isValid())
        count = m_model.data()->CombinedParameter().size();
    else{
        qreal * pos = static_cast<qreal *>(parent.internalPointer());

        if(pos == m_null)
            count = m_model.data()->CombinedParameter()[parent.row()].second.size();
        else
            count = 0;
    }
    */
    return count;
}

QVariant ParameterTree::data(const QModelIndex& index, int role) const
{
    QVariant data;
    if (!index.isValid())
        return data;

    qreal* pos = static_cast<qreal*>(index.internalPointer());

    if (role == Qt::DisplayRole) {
        qDebug() << *pos << pos;
        if (pos == m_null)
            data = "1"; //m_model.data()->CombinedParameter()[index.row()].first;
        else
            data = "blub";
    }
    return data;
}

QModelIndex ParameterTree::index(int row, int column, const QModelIndex& parent) const
{
    QModelIndex index;

    if (!hasIndex(row, column, parent))
        index = QModelIndex();

    if (!parent.isValid()) {
        index = createIndex(row, column, m_null);
    } else {
        //  qDebug() << parent.row() << row << &m_model.data()->CombinedParameter()[parent.row()].first << m_model.data()->CombinedParameter()[parent.row()].first;
        //  QVector<MMParameter> parm = m_model.data()->CombinedParameter();
        //qreal *value = &parm[parent.row()].first;
        //index = createIndex(row, column, value);
    }
    return index;
}

QModelIndex ParameterTree::parent(const QModelIndex& child) const
{
    QModelIndex index;

    if (!child.isValid())
        return index;
    /*
    qreal * pos = static_cast<qreal *>(child.internalPointer());
    if(pos != m_null)
    {
        for(int i = 0; i < m_model.data()->CombinedParameter().size(); ++i)
        {
            for(int j = 0; j < m_model.data()->CombinedParameter()[i].second.size(); ++j)
                if(&m_model.data()->CombinedParameter()[i].first == pos)
                {
                    index = createIndex(i, 0, m_null);
                }
        }
    }
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
