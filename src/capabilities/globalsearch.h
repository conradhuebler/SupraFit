/*
 * <one line to give the program's name and a brief idea of what it does.>
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

#ifndef GLOBALSEARCH_H
#define GLOBALSEARCH_H

#include "src/core/models.h"

#include <QtCore/QVector>
#include <QtCore/QList>
#include <QtCore/QPointF>
#include <QtCore/QObject>

class Minimizer;
class AbstractTitrationModel;

struct GlobalSearchResult
{
    QVector< QVector<double > > m_input;  
    QVector< double > m_error;
    QVector< double > m_corr_coeff;
    QVector< QJsonObject > m_models;
};

struct VisualData
{
    QVector<QVector<qreal > > data;
};

class GlobalSearch : public QObject
{
    Q_OBJECT
    
public:
    GlobalSearch(QObject *parent = 0);
    ~GlobalSearch();
    inline void setParameter(const QVector<QVector< qreal > > &parameter) { m_parameter = parameter; }
    inline void setOptimizationFlags(OptimizationType type) { m_type = type; }
    inline void setModel(const QSharedPointer<AbstractTitrationModel> model) { m_model = model->Clone(); }
    inline void setOptimize(bool optimize) { m_optimize = optimize; }
    inline void setInitialGuess(bool guess) { m_initial_guess = guess; }
    inline QVector<QList<qreal > > FullList() const { return m_full_list; }
    QList<QList<QPointF> >  LocalSearch();
    QList<QJsonObject > SearchGlobal();
    QVector<VisualData> Create2DPLot();
    
public slots:
    void Interrupt();
    
private:
    QVector<QVector<double> > ParamList();
    void ConvertList(const QVector<QVector<double> >& full_list, QVector<double > &error);
    void Scan(const QVector< QVector<double > >& list);
    QVector<QVector < qreal > > m_parameter;
    quint64 m_time_0;
    OptimizationType m_type;
    int m_time;
    QList<QJsonObject > m_models_list;
    QSharedPointer<AbstractTitrationModel> m_model;
    QList< QList<QPointF> > m_series;
    QVector<QList<double> > m_full_list;
    GlobalSearchResult last_result;
    double error_max;
    QSharedPointer<Minimizer> m_minimizer;
    bool m_allow_break, m_optimize, m_initial_guess;
    QVector<VisualData> m_3d_data;
    
signals:
    void SingeStepFinished(int time);
    void setMaximumSteps(int maxsteps);
};

#endif // GLOBALSEARCH_H
