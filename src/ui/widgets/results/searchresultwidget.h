/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2017 - 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <QtWidgets/QWidget>

#include "src/capabilities/globalsearch.h"

class AbstractModel;
class GlobalSearch;

class ChartView;
class ScientificBox;

class QCheckBox;
class QPushButton;
class QTableView;
class QTabWidget;

class SearchResultWidget : public QWidget {
    Q_OBJECT

public:
    SearchResultWidget(QPointer<GlobalSearch> globalsearch, const QSharedPointer<AbstractModel> model, QWidget* parent);
    ~SearchResultWidget();

private:
    QTableView* BuildList();
    QWidget* BuildContour();

    QList<QJsonObject> m_models;
    QTableView* m_table;
    QWidget* m_contour;
    QTabWidget* m_central_widget;
    QSharedPointer<AbstractModel> m_model;
    QCheckBox* m_valid;
    ScientificBox* m_threshold;
    QPushButton *m_export, *m_switch;
    QVector<QList<qreal>> m_input;
    QPointer<GlobalSearch> m_globalsearch;

    QList<GSResult> m_results;

private slots:
    void rowSelected(const QModelIndex& index);
    void ShowContextMenu(const QPoint& pos);
    void ExportModels();
    void SwitchView();

signals:
    void LoadModel(const QJsonObject& object);
    void AddModel(const QJsonObject& object);
};
