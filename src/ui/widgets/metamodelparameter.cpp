/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2018 - 2022 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/core/models/models.h"

#include "src/ui/guitools/chartwrapper.h"

#include <QtCore/QAbstractItemModel>
#include <QtCore/QFile>
#include <QtCore/QMimeData>
#include <QtCore/QSharedPointer>

#include <QtGui/QAction>
#include <QtGui/QPainter>
#include <QtGui/QTextDocument>

#include <QtWidgets/QGridLayout>
#include <QtWidgets/QTreeView>

#include "metamodelparameter.h"

int ParameterTree::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 1;
    else //Q_UNUSED(parent)
        return 2;
}

int ParameterTree::rowCount(const QModelIndex& parent) const
{

    int count = 0;

    if (!parent.isValid())
        count = m_model.toStrongRef().data()->CombinedParameter().size();
    else{
        qreal * pos = static_cast<qreal *>(parent.internalPointer());

        if(pos == m_null)
            count = m_model.toStrongRef().data()->CombinedParameter()[parent.row()].second.size();
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
                for (int i = 0; i < m_model.toStrongRef().data()->CombinedParameter().size(); ++i) {
                    if (pos == &m_model.toStrongRef().data()->CombinedParameter(i)->first) {
                        data = QString::number(i) + " " + QString::number(index.row());
                    }
                }
            }
        }
    }
    mimeData->setText(data);
    return mimeData;
}
bool ParameterTree::canDropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& index) const
{
    Q_UNUSED(action)

    if (!index.isValid())
        return false;

    QString string = data->text();
    QStringList list = string.split(" ");
    if (list.size() != 2)
        return false;

    if (row == -1 && column == -1 && !index.isValid()) {
        if (list[1] == "-1")
            return false;
    }

    if (index.row() == row && index.column() == column)
        return false;
    return true;
}

bool ParameterTree::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& index)
{
    Q_UNUSED(action)
    QString string = data->text();
    if (!index.isValid())
        return false;

    qreal* pos = static_cast<qreal*>(index.internalPointer());
    int parameter;
    if (pos == m_null)
        parameter = index.row();
    else {
        for (int i = 0; i < m_model.toStrongRef().data()->CombinedParameter().size(); ++i) {
            if (pos == &m_model.toStrongRef().data()->CombinedParameter(i)->first) {
                parameter = i;
            }
        }
    }

    QStringList list = string.split(" ");
    if (list.size() != 2)
        return false;

    if (row == -1 && column == -1 && !index.isValid()) {
        if (list[1] == "-1")
            return false;
        else
            m_model.toStrongRef().data()->MoveSingleParameter(list[0].toInt(), list[1].toInt());
        return true;
    }
    if (list[1].toInt() == -1) {
        m_model.toStrongRef().data()->MoveParameterList(list[0].toInt(), parameter);
    } else
        m_model.toStrongRef().data()->MoveSingleParameter(list[0].toInt(), list[1].toInt(), parameter);

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
        if (index.row() < m_model.toStrongRef().data()->CombinedParameter().size()) {
            if (pos == m_null) {

                QString name;
                if (index.column() == 0)
                    name = tr("%1").arg(m_model.toStrongRef().data()->CombinedParameter()[index.row()].first);
                else
                    name = tr("%1").arg(m_model.toStrongRef().data()->CombinedParameter()[index.row()].second.size());

                if (m_model.toStrongRef().data()->CombinedParameter()[index.row()].second.size() == 1) {
                    MMParameter param = m_model.toStrongRef().data()->CombinedParameter()[index.row()];
                    if (param.second.size() == 0)
                        return data;

                    QSharedPointer<AbstractModel> model = m_model.toStrongRef().data()->Models()[param.second[0][0]];
                    if (index.column() == 1) {
                        name += " | " + model->ProjectTitle() + " : " + model->Name() + tr(" [%1]").arg(model->GlobalParameterName(param.second[0][2]));
                    } else {
                        if (param.second[0][1] == 0)
                            name += ""; //" | " + model->ProjectTitle() + " : " + model->Name() + tr(" [%1]").arg(model->GlobalParameterName(param.second[0][2]));
                        else
                            name += " | " + model->ProjectTitle() + " : " + model->Name() + tr(" [%1]").arg(model->LocalParameterName(param.second[0][2]));
                    }
                }
                data = name;
            } else {
                for (int i = 0; i < m_model.toStrongRef().data()->CombinedParameter().size(); ++i) {
                    if (pos == &m_model.toStrongRef().data()->CombinedParameter(i)->first) {
                        const MMParameter* param = m_model.toStrongRef().data()->CombinedParameter(i);

                        if (index.row() >= param->second.size()) {
                            data = "Dr Strange, the bug hunter redeems SupraFit until the bug is fixed ... . He goes away after optimising.";
                            continue;
                        }

                        QSharedPointer<AbstractModel> model = m_model.toStrongRef().data()->Models()[param->second[index.row()][0]];
                        QString name;

                        if (param->second[index.row()][1] == 0)
                            name = model->GlobalParameterName(param->second[index.row()][2]);
                        else
                            name = model->LocalParameterName(param->second[index.row()][2]);
                        if (index.column() == 0)
                            data = model->ProjectTitle() + " : " + model->Name() + " " + name;
                    }
                }
            }
        }
    } else if (role == Qt::BackgroundRole) {
        if (index.row() < m_model.toStrongRef().data()->CombinedParameter().size()) {
            if (!index.parent().isValid()) {
                MMParameter param = m_model.toStrongRef().data()->CombinedParameter()[index.row()];
                if (param.second.size() > 1) {
                    QLinearGradient gradient(QPointF(0, 100), QPointF(1000, 100));
                    for (int i = 0; i < param.second.size(); ++i) {
                        QColor color = m_linked_list->value(m_model.toStrongRef().data()->Models()[param.second[i][0]]);
                        gradient.setColorAt(i / double(m_model.toStrongRef().data()->CombinedParameter().size()), color);
                    }
                    return QBrush(gradient);
                } else if (param.second.size() == 1) {
                    QColor color = m_linked_list->value(m_model.toStrongRef().data()->Models()[param.second[0][0]]);
                    return color;
                }
            } else if (index.parent().isValid()) {
                MMParameter param = m_model.toStrongRef().data()->CombinedParameter()[index.parent().row()];
                // qDebug() << index.row() << index.column()  << param;
                QColor color = m_linked_list->value(m_model.toStrongRef().data()->Models()[param.second[index.row()][0]]).lighter();
                return color;
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

    m_model.toStrongRef().data()->SetSingleParameter(value.toDouble(), index.row());
    m_model.toStrongRef().data()->Calculate();

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
        qreal* value = const_cast<qreal*>(&m_model.toStrongRef().data()->CombinedParameter(parent.row())->first);
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
        for (int i = 0; i < m_model.toStrongRef().data()->CombinedParameter().size(); ++i) {
            for (int j = 0; j < m_model.toStrongRef().data()->CombinedParameter()[i].second.size(); ++j)
                if (&m_model.toStrongRef().data()->CombinedParameter(i)->first == pos) {
                    index = createIndex(i, 0, m_null);
                }
        }
    }

    return index;
}

void ModelParameterEntry::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QStyleOptionViewItem options = option;
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

QSize ModelParameterEntry::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    if (index.parent().isValid())
        return QSize(50, 50);
    else
        return QSize(100, 50);
}
/*
void ModelParameterEntry::initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const
{
    QStyledItemDelegate::initStyleOption(option, index);
    option->font.setPointSize(60);
}*/

MetaModelParameter::MetaModelParameter(QSharedPointer<AbstractModel> model, QHash<QSharedPointer<AbstractModel>, QColor>* linked_list)
    : m_linked_list(linked_list)
{
    m_model = qSharedPointerCast<MetaModel>(model);
    setUi();
}

MetaModelParameter::~MetaModelParameter()
{
    m_model.clear();
}

void MetaModelParameter::setUi()
{
    m_layout = new QGridLayout;

    m_tree = new QTreeView;
    m_treemodel = new ParameterTree(m_model, m_linked_list);
    m_tree->setModel(m_treemodel);

    m_tree->setDragEnabled(true);

    m_tree->setAcceptDrops(true);
    m_tree->setDropIndicatorShown(true);
    m_tree->setDragDropMode(QAbstractItemView::DragDrop);
    m_tree->setItemDelegate(new ModelParameterEntry());
    m_tree->setContextMenuPolicy(Qt::ActionsContextMenu);
    m_tree->setColumnWidth(0, 400);
    QAction* action = new QAction(tr("Split Parameter"));
    m_tree->addAction(action);
    connect(action, &QAction::triggered, this, &MetaModelParameter::SplitParameter);

    m_layout->addWidget(m_tree, 0, 0);

    setLayout(m_layout);

    connect(m_model.toStrongRef().data(), &MetaModel::ModelAdded, this, [this]() {
        m_treemodel->layoutChanged();
    });

    connect(m_model.toStrongRef().data(), &MetaModel::ModelRemoved, this, [this]() {
        m_treemodel->layoutChanged();
    });

    connect(m_model.toStrongRef().data(), &MetaModel::ParameterSorted, this, [this]() {
        m_treemodel->layoutChanged();
    });

    connect(m_model.toStrongRef().data(), &MetaModel::Recalculated, this, [this]() {
        m_treemodel->layoutChanged();
    });
}

void MetaModelParameter::SplitParameter()
{
    const QModelIndex index = m_tree->currentIndex();
    if (m_treemodel->parent(index).isValid())
        return;
    m_model.toStrongRef().data()->MoveSingleParameter(index.row());
}
