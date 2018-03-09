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

#include "abstractsearchclass.h"
#include "weakenedgridsearch.h"

#include <QtCore/QObject>
#include <QtCore/QRunnable>

const int update_intervall = 100;


class MoCoConfig : public AbstractConfig
{
public:
    WGSConfig cv_config;  
    int mc_steps = 10000;
    qreal box_multi = 1.5;
    qreal maxerror = 0; 
    qreal confidence = 95;
    qreal f_value = 0;
    bool fisher_statistic = false;
};

class AbstractModel;

class MCThread : public AbstractSearchThread
{
  Q_OBJECT
  
public:
    inline MCThread( ) : AbstractSearchThread() { }
    inline ~MCThread() { }
    void setModel(QSharedPointer<AbstractModel> model) { m_model = model->Clone(); }
    void run();
    QList<QJsonObject > Results() const { return m_results; }
    inline void setMaxSteps(int steps) { m_maxsteps = steps; }
    inline void setBox(const QVector<QVector<qreal> > &box) { m_box = box; }
   
private:
    QSharedPointer<AbstractModel> m_model;
    QList<QJsonObject > m_results;
    int m_maxsteps;
    QVector<QVector<qreal> > m_box;
};

class FCThread : public AbstractSearchThread
{
  Q_OBJECT

public:
    FCThread(MoCoConfig config, int parameter) : AbstractSearchThread(), m_config(config), m_parameter(parameter) { }
    ~FCThread() { }

    void setModel(QSharedPointer<AbstractModel> model) { m_model = model->Clone(); }
    void run();
    inline qreal Lower() const { return m_lower; }
    inline qreal Upper() const { return m_upper; }

private:
    qreal m_lower, m_upper;
    int m_parameter;
    qreal SingleLimit(int parameter_id, int direction);
    MoCoConfig m_config;

};


class ModelComparison : public AbstractSearchClass
{
    Q_OBJECT

public:
    ModelComparison(MoCoConfig config, QObject *parent = 0);
    ~ModelComparison();
    bool Confidence();
    bool FastConfidence();
    inline qreal Area() const { return m_ellipsoid_area; }
     
private:
    void StripResults(const QList<QJsonObject>& results);
    void MCSearch(const QVector<QVector<qreal> >& box);
    double SingleLimit(int parameter_id, int direction = 1);
    virtual QJsonObject Controller() const override;

    QVector<QVector<qreal> > MakeBox();
    MoCoConfig m_config;
    QJsonObject m_box, m_controller;
    double m_effective_error, m_box_area, m_ellipsoid_area;
    QVector<QList<qreal > > m_data_vec;
};
