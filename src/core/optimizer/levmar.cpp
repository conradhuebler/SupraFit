/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016  Conrad Hübler <Conrad.Huebler@gmx.net>
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
#include "src/global_config.h"

#ifdef USE_levmarOptimizer

#include <QtGlobal>
#include <QtMath>
#include <QDebug>
#include <cmath>
#include <QPair>
#include "src/core/AbstractModel.h"
#include "src/core/libmath.h"
#include <iostream>

#include <levmar/levmar.h>


struct mydata{
    QSharedPointer<AbstractTitrationModel> model;
};

void TitrationModel(double *p, double *x, int m, int n, void *data)
{
    
    Q_UNUSED(n);
    struct mydata *dptr;
    
    dptr=(struct mydata *)data; 
    QVector<qreal> parameter;
    for(int i = 0; i < m; ++i)
        parameter << p[i];
    if(parameter.size() == 2)
        qDebug() << parameter;
    dptr->model->setParamter(parameter);
    
    dptr->model->CalculateSignal();
    QVector<qreal > x_var = dptr->model->getCalculatedSignals();
    for(int i = 0; i < x_var.size(); ++i)
        x[i] = x_var[i];

}

int MinimizingComplexConstants(QSharedPointer<AbstractTitrationModel> model, int max_iter, QVector<qreal > &param, const OptimizerConfig &config)
{
    double opts[LM_OPTS_SZ], info[LM_INFO_SZ];
    opts[0]=config.LevMar_mu; opts[1]=config.LevMar_Eps1; opts[2]=config.LevMar_Eps2; opts[3]=config.LevMar_Eps3;
    opts[4]=config.LevMar_Delta;
    struct mydata data;
    data.model = model;
    double *x = new double[data.model->DataPoints()*data.model->SignalCount()];
    
    QVector<qreal > x_var = data.model->getSignals(data.model->ActiveSignals());
    for(int i = 0; i < x_var.size(); ++i)
        x[i] = x_var[i];
    
    
    QVector<double > parameter = param;
    
    QString message = QString();
    message += "Starting Levenberg-Marquardt for " + QString::number(parameter.size()) + " parameters:\n";
    message += "Old vector : ";
    foreach(double d, parameter)
    {
        message += QString::number(d) + " ";
    }
    message += "\n";
    model->Message(message, 5);
    
    qDebug() << parameter;
    double *p = new double[parameter.size()];
    for(int i = 0; i < parameter.size(); ++i)
    {
        p[i] =  parameter[i];
    }
    
    int m =  parameter.size();
    int n = model->DataPoints()*model->SignalCount();
    
    int nums = dlevmar_dif(TitrationModel, p, x, m, n, max_iter, opts, info, NULL, NULL, (void *)&data);
    QString result;
    result += "Levenberg-Marquardt returned in  " + QString::number(info[5]) + " iter, reason "+ QString::number(info[5]) + ", sumsq " + QString::number(info[5]) + "\n";
    result += "New vector:";    
    param.clear();
    for(int i = 0; i < m; ++i)
    {
        param << p[i];
        result +=  QString::number(p[i]) + " ";
    }
    result += "\n";
    model->Message(result, 4);
    
    delete[] x;
    delete[] p;
    return nums;
}

#endif
