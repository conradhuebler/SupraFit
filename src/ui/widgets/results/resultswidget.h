/*
 * <one line to give the program's name and a brief idea of what it does.>
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

class AbstractSearchClass;
class AbstractModel;
class ChartWrapper;

class QJsonObject;

class ResultsWidget : public QWidget {
    Q_OBJECT

public:
    ResultsWidget(const QJsonObject& data, QSharedPointer<AbstractModel> model, ChartWrapper* wrapper, const QList<QJsonObject>& models = QList<QJsonObject>());
    ~ResultsWidget();

private:
    QWidget* MonteCarloWidget();
    QWidget* ReductionWidget();
    QWidget* ModelComparisonWidget();
    QWidget* GridSearchWidget();
    QWidget* SearchWidget();
    void setUi();
    inline QSize ChartSize() const { return QSize(400, 300); }

    QJsonObject m_data;

    ChartWrapper* m_wrapper;
    QList<QJsonObject> m_models;
    QWidget* m_widget;
    QLabel* m_confidence_label;
    QSharedPointer<AbstractModel> m_model;

private slots:
    void WriteConfidence(const QJsonObject& data);
};
