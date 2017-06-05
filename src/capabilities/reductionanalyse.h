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

#ifndef REDUCTIONANALYSE_H
#define REDUCTIONANALYSE_H

#include "src/capabilities/abstractsearchclass.h"
#include "src/core/AbstractModel.h"

#include <QtCore/QObject>
#include <QtCore/QSharedPointer>

class AbstractModel;
class Minimizer;

class ReductionAnalyse : public QObject
{
    Q_OBJECT
    
public:
    ReductionAnalyse(OptimizerConfig config, OptimizationType type);
    ~ReductionAnalyse();
    void setModel(const QSharedPointer<AbstractModel > model) { m_model = model->Clone(); }
    void CrossValidation();
    void PlainReduction();
    inline QVector<QList<QPointF >> Series() { return m_series; }
private:
    QSharedPointer<AbstractModel> m_model;
    AbstractConfig m_config;
    QVector<QList<QPointF >> m_series;
};

#endif // REDUCTIONANALYSE_H
