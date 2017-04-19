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

#include "abstractsearchclass.h"

#include "src/core/models.h"

#include <QtCore/QObject>
#include <QtCore/QList>
#include <QtCore/QPointF>
#include <QtCore/QVector>

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

class GSConfig : public AbstractConfig
{
public:
    inline GSConfig(OptimizerConfig config, OptimizationType type) : AbstractConfig(config, type) { }
    inline GSConfig() { }
    bool initial_guess = true;
    bool optimize = true;
    QVector<QVector< qreal > > parameter;
};

class GlobalSearch : public AbstractSearchClass
{
    Q_OBJECT
    
public:
    GlobalSearch(GSConfig config = GSConfig(), QObject *parent = 0);
    GlobalSearch(QObject *parent = 0);
    ~GlobalSearch();
    inline void setConfig(const GSConfig &config) { m_config = config; }
    inline QVector<QList<qreal > > InputList() const { return m_full_list; }
    QList<QList<QPointF> >  LocalSearch();
    QList<QJsonObject > SearchGlobal();
    QVector<VisualData> Create2DPLot();
    void ExportResults(const QString &filename, double threshold, bool allow_invalid);
    
public slots:
    void Interrupt();
    
private:
    QVector<QVector<double> > ParamList();
    void ConvertList(const QVector<QVector<double> >& full_list, QVector<double > &error);
    void Scan(const QVector< QVector<double > >& list);
//     QVector<QVector < qreal > > m_parameter;
    quint64 m_time_0;
//     OptimizationType m_type;
    int m_time, m_max_count;
    QVector<QList<double> > m_full_list;
    GlobalSearchResult last_result;
    double error_max;
    QSharedPointer<Minimizer> m_minimizer;
    bool m_allow_break; //, m_optimize, m_initial_guess;
    QVector<VisualData> m_3d_data;
    GSConfig m_config;
};

#endif // GLOBALSEARCH_H
