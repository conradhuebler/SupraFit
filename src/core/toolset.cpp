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


#include <QtCore/QJsonObject>
#include <QtCore/QString>
#include <QtCore/QPointF>
#include <QtCore/QVector>

#include <QDebug>
#include <Eigen/Dense>

#include <functional>

#include "toolset.h"
typedef Eigen::VectorXd Vector;

namespace ToolSet{
    
    QString DoubleVec2String(const QVector<qreal > &vector, const QString &str)
    {
        QString string;
        for(int i = 0; i < vector.size(); ++i)    
            string += QString::number(vector[i]) + str;
        
        return string.left(string.length() - 1);
    }
    
    QString DoubleList2String(const QList<qreal > &vector, const QString &str)
    {
        QString string;
        for(int i = 0; i < vector.size(); ++i)    
            string += QString::number(vector[i]) + str;
        
        return string.left(string.length() - 1);
    }
    
    QString DoubleList2String(const Vector &vector, const QString &str)
    {
        QString string;
        for(int i = 0; i < vector.rows(); ++i)    
            string += QString::number(vector(i)) + str;
        
        return string.left(string.length() - 1);
    }
    QStringList DoubleList2StringList(const Vector &vector)
    {
        QStringList list;
        for(int i = 0; i < vector.rows(); ++i)    
            list << QString::number(vector(i));
        return list;
    }
    
    QVector<qreal > String2DoubleVec(const QString &str)
    {
        QVector<qreal > vector;
        QStringList nums = str.split(" ");
        for(const QString &string: qAsConst(nums))
            vector << string.toDouble();
        return vector;
    }
    
    QList<qreal > String2DoubleList(const QString &str)
    {
        QList<qreal > vector;
        QStringList nums = str.split(" ");
        for(const QString &string: qAsConst(nums))
            vector << string.toDouble();
        return vector;
    }
    
    QString bool2YesNo(bool var) 
    {
        if(var)
            return QString("yes");
        else
            return QString("No");
    }
    
    qreal scale(qreal value)
    {
        qreal pot;
        return scale(value, pot);
    }
    
    qreal scale(qreal value, qreal &pow)
    {
        if(qAbs(value) < 1 && value)
        {
            while(qAbs(value) < 1)
            {
                pow /= 10;
                value *= 10;
            }
        }
        else if(qAbs(value) > 10)
        {
            while(qAbs(value) > 10)
            {
                pow *= 10;
                value /= 10;
            }
        }
        return value;
    }
    
    qreal ceil(qreal value)
    {
        double pot = 1;
        value = scale(value, pot);
        int integer = int(value) + 1;    
        if(value < 0)
            integer -= 1;
        return qreal(integer)*pot;
    }
    
    qreal floor(qreal value)
    {
        double pot = 1;
        value = scale(value, pot);
        
        int integer = int(value);
        if(value < 0)
            integer -= 1;
        return qreal(integer)*pot;
    }
    
    QVector<QPair<qreal, int > > List2Histogram(const QVector<qreal> &vector, int bins, qreal min, qreal max)
    {
        
        
        if(min == max)
        {
            for(int i = 0; i < vector.size(); ++i)   
            {
                min = qMin(min, vector[i]);
                max = qMax(max, vector[i]);
            }  
        }
        min = floor(min);
        max = ceil(max);
        if(bins == 0)
        {
            if(vector.size() > 1e5)
                bins = vector.size()/1e4;
            else if(vector.size() < 1e2)
                bins = vector.size()/1e2;
            else
                bins = 10;
        }
        
        QVector<QPair<qreal, int > > histogram;
        double h=(max-min)/bins;
        QVector<double > x(bins,0);
        QVector<int> counter(bins, 0);

        for(int j=0;j<bins;j++)
        {
            x[j] = min+h/2.+j*h;  
            QPair<qreal, int> bin;
            bin.second = 0;
            bin.first = min+h/2.+j*h;
            histogram << bin;
        }
        for(int i = 0; i < vector.size(); ++i)
        {
            int jStar = std::floor( (vector[i]-min)/h ); // use the floor function to get j* for the nearest point to x_j* to phi
            if(jStar>=bins || jStar<0)continue; // if you are outside the grid continue
                counter[jStar]++;
                histogram[jStar].second++;
        }
        return histogram;
    }

    ConfidenceBar Confidence(const QList<qreal > &list, qreal error)
    {   
        ConfidenceBar result;

        if(error == 0)
        {
            result.lower = list.first();
            result.upper = list.last();
        }else if(error == 100)
        {
            int max = list.size();
            int pos = max/2;
            if(max % 2 == 1)
            {
                result.lower = list[pos];
                result.upper = list[pos+1];
            }else{
                result.lower = list[pos];
                result.upper = list[pos];
            }
        }else{
            int max = list.size();
            int pos_upper = max*(1-error/100);
            int pos_lower = max*(error/100);
            result.lower = list[pos_lower];
            result.upper = list[pos_upper];
        }
        return result;
    }
    
    QList<QPointF> fromModelsList(const QList<QJsonObject> &models)
    {
        QList<QPointF> series;
        for(const QJsonObject &obj : qAsConst(models))
        {
            QJsonObject constants = obj["data"].toObject()["constants"].toObject();
            series << QPointF(constants[QString::number(0)].toString().toDouble(), constants[QString::number(1)].toString().toDouble());
        }
        return series;
    }
    
    qreal SimpsonIntegrate(qreal lower, qreal upper, std::function<qreal(qreal, const QVector<qreal >)> function, const QVector<qreal > &parameter)
    {
        qreal integ = 0;
        qreal delta = 1E-4;
        for(qreal x = lower; x <= (upper- delta); x += delta)
        {
            qreal b = x + delta;
            integ += (b-x)/6*(function(x,parameter)+4*function((x+b)/2,parameter)+function(b,parameter));
        }
        return integ;
    }
}
