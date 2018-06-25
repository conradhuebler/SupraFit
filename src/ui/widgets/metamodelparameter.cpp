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
#include <QtCore/QMimeData>
#include <QtCore/QSharedPointer>

#include <QtGui/QPainter>
#include <QtGui/QTextDocument>

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

    if (!parent.isValid())
        count = m_model.data()->CombinedParameter().size();
    else{
        qreal * pos = static_cast<qreal *>(parent.internalPointer());

        if(pos == m_null)
            count = m_model.data()->CombinedParameter()[parent.row()].second.size();
        else
            count = 0;
    }

    return count;
}

QMimeData* ParameterTree::mimeData(const QModelIndexList& indexes) const
{
    QMimeData* mimeData = new QMimeData();

    QString data;

    for (const QModelIndex& index : qAsConst(indexes)) {
        if (index.isValid()) {

            qreal* pos = static_cast<qreal*>(index.internalPointer());

            if (pos == m_null)
                data = QString::number(index.row()) + " -1";
            else {
                for (int i = 0; i < m_model.data()->CombinedParameter().size(); ++i) {
                    if (pos == &m_model.data()->CombinedParameter(i)->first) {
                        data = QString::number(i) + " " + QString::number(index.row());
                    }
                }
            }
        }
    }
    mimeData->setText(data);
    return mimeData;
}

bool ParameterTree::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
{
    QString string = data->text();

    qreal* pos = static_cast<qreal*>(parent.internalPointer());
    int parameter;
    if (pos == m_null)
        parameter = parent.row();
    else {
        for (int i = 0; i < m_model.data()->CombinedParameter().size(); ++i) {
            if (pos == &m_model.data()->CombinedParameter(i)->first) {
                parameter = i;
            }
        }
    }

    QStringList list = string.split(" ");
    if (list.size() != 2)
        return false;

    if (list[1].toInt() == -1) {
        m_model.data()->MoveParameterList(list[0].toInt(), parameter);
    } else
        m_model.data()->MoveSingleParameter(list[0].toInt(), list[1].toInt(), parameter);

    layoutChanged();

    return true;
}

QVariant ParameterTree::data(const QModelIndex& index, int role) const
{
    QVariant data;
    if (!index.isValid())
        return data;

    qreal* pos = static_cast<qreal*>(index.internalPointer());

    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        if (pos == m_null)
            data = m_model.data()->CombinedParameter()[index.row()].first;
        else {
            for (int i = 0; i < m_model.data()->CombinedParameter().size(); ++i) {
                if (pos == &m_model.data()->CombinedParameter(i)->first) {
                    const MMParameter* param = m_model.data()->CombinedParameter(i);
                    QSharedPointer<AbstractModel> model = m_model.data()->Models()[param->second[index.row()][0]];
                    QString name;

                    if (param->second[index.row()][1] == 0)
                        name = model->GlobalParameterName(param->second[index.row()][2]);
                    else
                        name = model->LocalParameterName(param->second[index.row()][2]);
                    data = model->ProjectTitle() + ":" + model->Name() + " " + name;
                }
            }
        }
    }
    return data;
}

bool ParameterTree::setData(const QModelIndex& index, const QVariant& value, int role)
{
    qreal* pos = static_cast<qreal*>(index.internalPointer());

    if (role != Qt::EditRole || pos != m_null)
        return false;

    m_model.data()->SetSingleParameter(value.toDouble(), index.row());
    m_model.data()->Calculate();

    emit dataChanged(index, index);

    return true;
}

QModelIndex ParameterTree::index(int row, int column, const QModelIndex& parent) const
{
    QModelIndex index;

    if (!hasIndex(row, column, parent))
        index = QModelIndex();

    if (!parent.isValid()) {
        index = createIndex(row, column, m_null);
    } else {
        qreal* value = const_cast<qreal*>(&m_model.data()->CombinedParameter(parent.row())->first);
        index = createIndex(row, column, value);
    }
    return index;
}

QModelIndex ParameterTree::parent(const QModelIndex& child) const
{
    QModelIndex index;

    if (!child.isValid())
        return index;

    qreal * pos = static_cast<qreal *>(child.internalPointer());
    if(pos != m_null)
    {
        for(int i = 0; i < m_model.data()->CombinedParameter().size(); ++i)
        {
            for(int j = 0; j < m_model.data()->CombinedParameter()[i].second.size(); ++j)
                if (&m_model.data()->CombinedParameter(i)->first == pos) {
                    index = createIndex(i, 0, m_null);
                }
        }
    }

    return index;
}

void ModelParameterEntry::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QStyleOptionViewItemV4 options = option;
    initStyleOption(&options, index);

    painter->save();

    QTextDocument doc;
    doc.setHtml(options.text);

    options.text = "";
    options.widget->style()->drawControl(QStyle::CE_ItemViewItem, &options, painter);

    painter->translate(options.rect.left(), options.rect.top());
    QRect clip(0, 0, options.rect.width(), options.rect.height());
    doc.drawContents(painter, clip);

    painter->restore();
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

    m_tree->setDragEnabled(true);

    m_tree->setAcceptDrops(true);
    m_tree->setDropIndicatorShown(true);
    m_tree->setDragDropMode(QAbstractItemView::DragDrop);
    m_tree->setItemDelegate(new ModelParameterEntry());
    m_layout->addWidget(m_tree, 0, 0);

    setLayout(m_layout);

    connect(m_model.data(), &MetaModel::ModelAdded, this, [this]() {
        m_treemodel->layoutChanged();
    });
    connect(m_model.data(), &MetaModel::Recalculated, this, [this]() {
        m_treemodel->layoutChanged();
    });
}
