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

#ifndef ADVANCEDSEARCH_H
#define ADVANCEDSEARCH_H

#include "src/global.h"
#include "src/core/AbstractModel.h"

#include <QtDataVisualization>

#include <QtCore/QWeakPointer>
#include <QtCore/QPointer>

#include <QtWidgets/QGroupBox>
#include <QtWidgets/QDialog>

class QLineEdit;
class Minimizer;
class QCheckBox;
class QPushButton;
class QDoubleSpinBox;
class QJsonObject;
class OptimizerFlagWidget;

struct GlobalSearchResult
{
    QVector< QVector<double > > m_input;  
    QVector< double > m_error;
    QVector< double > m_corr_coeff;
    QVector< QJsonObject > m_models;
};


class ParameterWidget : public QGroupBox
{
  Q_OBJECT
  
public:
    ParameterWidget(const QString &name, QWidget *parent = 0);
    double Min() const;
    double Max() const;
    double Step() const;
private:
    QPointer<QDoubleSpinBox > m_min, m_max, m_step;
};



class AdvancedSearch : public QDialog
{
    Q_OBJECT

public:
    AdvancedSearch(QWidget *parent = 0);
    ~AdvancedSearch();
    
    inline void setMinimizer(QWeakPointer<Minimizer> minimizer) { m_minimizer = minimizer; }
    inline void setModel(const QSharedPointer<AbstractTitrationModel> model) { m_model = model->Clone(); SetUi();}
    inline GlobalSearchResult  LastResult() const { return last_result; }
    inline QtDataVisualization::QSurfaceDataArray dataArray() const { return m_3d_data; }
    inline double MaxError() const { return error_max; }
    double MaxX() const;
    double MinX() const;
    double MaxY() const;
    double MinY() const;
    QList<QList<QPointF> >Series() const { return m_series; }
private:
    void SetUi();
    void Scan(const QVector< QVector<double > > &list);
    QSharedPointer<Minimizer> m_minimizer;
    QSharedPointer<AbstractTitrationModel> m_model;
    QPointer<QCheckBox > m_optim;
    QPointer<QPushButton > m_2d_search, m_1d_search, m_scan;
    GlobalSearchResult last_result;
    void ConvertList(const QVector< QVector<double > > &list,  QVector<double > &error);
    QtDataVisualization::QSurfaceDataArray m_3d_data;
    QList<QList<QPointF> > m_series;
    OptimizationType m_type;
    QPointer<OptimizerFlagWidget > m_optim_flags;
    double error_max;
    QVector< QVector<double > > ParamList() const;
    QList<QJsonObject > m_models_list;
    QVector<QPointer<ParameterWidget > > m_parameter_list;
private slots:
    void Create2DPlot();
    void LocalSearch();
    void GlobalSearch();
signals:
    void PlotFinished(int runtype);
    void MultiScanFinished(int runtype);
};

#endif // ADVANCEDSEARCH_H
