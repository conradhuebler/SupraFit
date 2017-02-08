/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2017  Conrad Hübler <Conrad.Huebler@gmx.net>
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

#ifndef MODELTABLEWIDGET_H
#define MODELTABLEWIDGET_H

#include <QtWidgets/QWidget>

class AbstractTitrationModel;
class QTableView;

class ModelTableWidget : public QWidget
{
    Q_OBJECT
    
public:
    ModelTableWidget();
    ~ModelTableWidget();
    void setModelList(const QList<QJsonObject> &list);
    void setModel(const QSharedPointer<AbstractTitrationModel> model){ m_model = model; }
private:
    QList<QJsonObject> m_list;
    QTableView *m_table;
    QSharedPointer<AbstractTitrationModel> m_model;
    
private slots:
    void rowSelected(QModelIndex index);
    
signals:
    void LoadModel(const QJsonObject &object);
};

#endif // MODELTABLEWIDGET_H
