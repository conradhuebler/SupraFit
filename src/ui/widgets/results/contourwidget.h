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

#include <QtWidgets/QWidget>

#include <QtCharts/QScatterSeries>

class AbstractModel;
class ListChart;

class ContourWidget : public QWidget {

    Q_OBJECT

public:
    ContourWidget();
    void MakePlot(int var_1, int var_2);
    void setData(const QList<QJsonObject> models, const QSharedPointer<AbstractModel> model);
public slots:
    inline void setConverged(bool converged)
    {
        m_converged = converged;
        Update();
    }
    inline void setValid(bool valid)
    {
        m_valid = valid;
        Update();
    }

private:
    void setUi();
    void CheckBox(int variable, int state);
    void Update();

    QWidget* VariWidget();

    QList<QJsonObject> m_models;
    QSharedPointer<AbstractModel> m_model;
    QtCharts::QScatterSeries* m_xy_series;

    ListChart* view;
    int m_var_1 = -1, m_var_2 = -1;
    QStringList m_names;

    bool m_converged = true, m_valid = true;
    void PointClicked(const QPointF& point);

signals:
    void Checked(int var_1, int var_2);
    void HideBox(int parameter);
    void CheckParameterBox(int parameter);
    void ModelClicked(int model);
};
