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

#pragma once

#include <QtCore/QAbstractItemModel>
#include <QtCore/QFile>
#include <QtCore/QSharedPointer>

#include <QtWidgets/QWidget>

class QTreeView;

class ParameterTree : public QAbstractItemModel {
    Q_OBJECT
public:
    inline ParameterTree(QWeakPointer<MetaModel> model)
    {
        m_model = model;
        m_null = &null;
        m_zero = new QVector<int>;
        (*m_zero) = { -1, -1, -1, -1 };
    }

    inline ~ParameterTree()
    {
        delete m_zero;
    }

    Qt::ItemFlags flags(const QModelIndex& index) const override
    {
        Q_UNUSED(index);
        Qt::ItemFlags flags;
        //if (m_checkable)
        flags = Qt::ItemIsDragEnabled | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable;
        //else
        //    flags = Qt::ItemIsEnabled;

        return flags;
    }

    /* Qt::DropActions supportedDragActions() const override
    {

    }*/

    virtual int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    virtual int rowCount(const QModelIndex& parent = QModelIndex()) const override;

    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;

    virtual QModelIndex parent(const QModelIndex& child) const override;

private:
    QWeakPointer<MetaModel> m_model;
    QVector<int>* m_zero;
    qreal null = -1;
    qreal* m_null;
};

class MetaModelParameter : public QWidget {
    Q_OBJECT
public:
    MetaModelParameter(QSharedPointer<AbstractModel> model);

signals:

public slots:

private:
    QTreeView* m_tree;
    QWeakPointer<MetaModel> m_model;
    QGridLayout* m_layout;
    ParameterTree* m_treemodel;
    void setUi();
};
