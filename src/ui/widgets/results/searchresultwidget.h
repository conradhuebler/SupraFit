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

#ifndef SearchResultWidget_H
#define SearchResultWidget_H

#include <QtWidgets/QWidget>

class ChartView;

class GlobalSearch;
class AbstractTitrationModel;
class QTableView;
class QPushButton;
class ScientificBox;
class QCheckBox;

class SearchResultWidget : public QWidget
{
    Q_OBJECT
    
public:
    SearchResultWidget(QPointer<GlobalSearch> globalsearch, const QSharedPointer<AbstractTitrationModel> model, QWidget *parent = 0);
    ~SearchResultWidget();
    inline void setModelList(const QList<QJsonObject> &models){m_models = models;};
    void setModel(const QSharedPointer<AbstractTitrationModel> model){ m_model = model; }
    void setInputList(const QVector<QList <qreal > >&input) { m_input = input; }
    
private:
    QList<QJsonObject> m_models;
    QTableView *m_table;
    ChartView *m_contour;
    QSharedPointer<AbstractTitrationModel> m_model;
    QCheckBox *m_valid;
    ScientificBox *m_threshold;
    QPushButton *m_export, *m_switch;
    QVector<QList <qreal > >m_input;
    QPointer<GlobalSearch> m_globalsearch;
    QTableView* BuildList();
    ChartView* BuildContour();
    
private slots:
    void rowSelected(const QModelIndex &index);
    void ShowContextMenu(const QPoint& pos);
    void ExportModels();
    void SwitchView();
    
signals:
    void LoadModel(const QJsonObject &object);
    void AddModel(const QJsonObject &object);
};

#endif // SearchResultWidget_H