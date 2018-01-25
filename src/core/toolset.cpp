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

#include <QtCore/QFile>
#include <QtCore/QJsonObject>
#include <QtCore/QString>
#include <QtCore/QPointF>
#include <QtCore/QVector>

#include <QDebug>
#include <Eigen/Dense>


#include <functional>
#include "jsonhandler.h"

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
    
    QList<QPointF> String2Points(const QString &str)
    {
        QList<QPointF> points;
        QStringList list = str.split(" ");
        for(const QString &point : qAsConst(list))
        {
                double x = 0, y = 0;
                QStringList l = point.split(",");
                if(l.size() != 2)
                    continue;
                x = l[0].remove("(").toDouble();
                y = l[1].remove(")").toDouble();
                points << QPointF(x,y);
        }
        return points;
    }
    QString Points2String(const QList<QPointF> &points)
    {
            QString string;
            for(int i = 0; i < points.size(); ++i)
                string += "(" + QString::number(points[i].x())+ "," + QString::number(points[i].y()) + ") ";
            return string;
    }
    
    QString bool2YesNo(bool var) 
    {
        if(var)
            return QString("Yes");
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
    
    void  Normalise(QVector<QPair<qreal, qreal> > &hist)
    {
        qreal max = 0;
        for(int i = 0; i < hist.size(); ++i)
            max = qMax(max, double(hist[i].second));
        
        for(int i = 0; i < hist.size(); ++i)
            hist[i].second = hist[i].second/max;
    }
    
    QVector<QPair<qreal, qreal > > List2Histogram(const QVector<qreal> &vector, int bins, qreal min, qreal max)
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
        
        QVector<QPair<qreal, qreal > > histogram;
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
        Normalise(histogram);
        return histogram;
    }

    SupraFit::ConfidenceBar Confidence(const QList<qreal > &list, qreal error)
    {   
        SupraFit::ConfidenceBar result;
        error /= 2;
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
    
    SupraFit::BoxWhisker BoxWhiskerPlot(const QList<qreal>& list)
    {
        SupraFit::BoxWhisker bw;
        int count = list.size();
        
        /* inspired by qt docs: Box and Whiskers Example
         * https://doc.qt.io/qt-5.10/qtcharts-boxplotchart-example.html
         */
        auto Median = [](const QList<qreal>& list, int begin, int end){
            int count = end - begin;
            if (count % 2) {
                return list.at(count / 2 + begin);
            } else {
                qreal right = list.at(count / 2 + begin);
                qreal left = list.at(count / 2 - 1 + begin);
                return (right + left) / 2.0;
            }
        };

        bw.median = Median(list, 0, count);
        bw.lower_quantile = Median(list, 0, count / 2);
        bw.upper_quantile = Median(list, count / 2 + (count % 2), count);
        bw.lower_whisker = bw.lower_quantile;
        bw.upper_whisker = bw.upper_quantile;
        /*
         * plagiate stopped
         */
        bw.count = count;
        qreal iqd = bw.upper_quantile - bw.lower_quantile;
        
        for(int i = 0; i < count; ++i)
        {
            bw.mean += list[i];
            if(list[i] < bw.median-3*iqd || list[i] > bw.median+3*iqd)
                bw.extreme_outliers << list[i];
            else if(list[i] < bw.median-1.5*iqd || list[i] > bw.median+1.5*iqd)
                bw.mild_outliers << list[i];
            else
            {
                bw.lower_whisker = qMin(list[i], bw.lower_whisker);
                bw.upper_whisker = qMax(list[i], bw.upper_whisker);
            }
        }
        bw.mean /= double(count);
        return bw;
    }

    QJsonObject Box2Object(const SupraFit::BoxWhisker& box)
    {
        QJsonObject object;
        object["lower_whisker"] = box.lower_whisker;
        object["upper_whisker"] = box.upper_whisker;
        object["lower_quantile"] = box.lower_quantile;
        object["upper_quantile"] = box.upper_quantile;
        object["median"] = box.median;
        object["mean"] = box.mean;
        object["count"] = box.count;
        object["extreme_outliers"] = DoubleList2String(box.extreme_outliers);
        object["mild_outliers"] = DoubleList2String(box.mild_outliers);
        return object;
    }

    SupraFit::BoxWhisker Object2Whisker(const QJsonObject& object)
    {
        SupraFit::BoxWhisker box;
        box.lower_whisker = object["lower_whisker"].toDouble();
        box.upper_whisker = object["upper_whisker"].toDouble();
        box.lower_quantile = object["lower_quantile"].toDouble();
        box.upper_quantile = object["upper_quantile"].toDouble();
        box.median = object["median"].toDouble();
        box.mean = object["mean"].toDouble();
        box.count = object["count"].toDouble();
        box.extreme_outliers = String2DoubleList(object["extreme_outliers"].toString());
        box.mild_outliers = String2DoubleList(object["mild_outliers"].toString());
        return box;
    }
    
    QList<QJsonObject> Model2Parameter(const QList<QJsonObject>& models, bool sort)
    {
        
        int globalcount, localcount, each_local;
        QJsonObject model = models.first()["data"].toObject();
        globalcount = model["globalParameter"].toObject().size() - 1;
        localcount  = model["localParameter"].toObject().size() - 1;
        QStringList local_names, global_names;
        local_names = model["localParameter"].toObject()["names"].toString().split("|");
        each_local  = local_names.count();
        global_names = model["globalParameter"].toObject()["names"].toString().split("|");
        QVector<QVector<qreal> >global(globalcount);
        QVector<QVector<qreal> >local(localcount*each_local);
        for(int i = 0; i < models.size(); ++i)
        {
            QJsonObject model = models[i]["data"].toObject();
            
            QJsonObject globalObject = model["globalParameter"].toObject();
            QJsonObject localObject = model["localParameter"].toObject();
            for(int j = 0; j < globalcount; ++j)
                global[j] << globalObject[QString::number(j)].toString().toDouble();
            
            
            for(int j = 0; j < localcount; ++j)
            {
                QList<qreal> values = String2DoubleList(localObject[QString::number(j)].toString());
                for(int k = 0; k < each_local; ++k)
                {
                    local[each_local*j+k] << values[k];
                }
            }
        }
        
        if(sort)
        {
            for(int i = 0; i < global.size(); ++i)
            {
                QVector<double > vector = global[i];
                std::sort(vector.begin(), vector.end());
                global[i] = vector;
            }
            for(int i = 0; i < local.size(); ++i)
            {
                QVector<double > vector = local[i];
                std::sort(vector.begin(), vector.end());
                local[i] = vector;
            }
        }
        
        QList<QJsonObject> parameter;
        
        for(int i = 0; i < globalcount; ++i)
        {
            QJsonObject object;
            QJsonObject data;
            data["raw"]= DoubleVec2String(global[i]);
            object["data"] = data;
            object["name"] = global_names[i];
            object["type"] = "Global Parameter";
            object["index"] = QString::number(i);
            parameter << object;
        }
        
        for(int i = 0; i < localcount; ++i)
        {
            for(int j = 0; j < each_local; ++j)
            {
                QJsonObject object;
                QJsonObject data;
                data["raw"] = DoubleVec2String(local[each_local*i+j]);
                object["data"] = data;  
                object["name"] = local_names[j];
                object["type"] = "Local Parameter";
                object["index"] = QString::number(j) + "|" + QString::number(i);
                parameter << object;
            }
        }
        return parameter;
    }
    
    void Parameter2Statistic(QList<QJsonObject>& parameter, const QPointer<AbstractModel> model, const QJsonObject &controller)
    {
        for(int i = 0; i < parameter.size(); ++i)
        {
            QList<qreal > list = ToolSet::String2DoubleList( parameter[i]["data"].toObject()["raw"].toString());
            if(parameter[i]["type"].toString() == "Global Parameter")
                parameter[i]["value"] = model->GlobalParameter(parameter[i]["index"].toString().toInt());
            else
            {
                QStringList index = parameter[i]["index"].toString().split("|");
                parameter[i]["value"] = model->LocalParameter(index[0].toInt(),index[1].toInt());
            }
            parameter[i]["boxplot"] = ToolSet::Box2Object(ToolSet::BoxWhiskerPlot(list));
            parameter[i]["controller"] = controller;
        }
    }
    
    QList<QPointF> fromModelsList(const QList<QJsonObject> &models, const QString &str)
    {
        QList<QPointF> series;
        for(const QJsonObject &obj : qAsConst(models))
        {
            QJsonObject constants = obj["data"].toObject()[str].toObject();
            series << QPointF(constants[QString::number(0)].toString().toDouble(), constants[QString::number(1)].toString().toDouble());
        }
        return series;
    }
    
    qreal SimpsonIntegrate(qreal lower, qreal upper, std::function<qreal(qreal, const QVector<qreal >)> function, const QVector<qreal > &parameter)
    {
        qreal integ = 0;
        qreal delta = 5E-4;
        for(qreal x = lower; x <= (upper- delta); x += delta)
        {
            qreal b = x + delta;
            integ += (b-x)/6*(function(x,parameter)+4*function((x+b)/2,parameter)+function(b,parameter));
        }
        return integ;
    }
    
     qreal finv(qreal p, int m, int n)
     {
         qreal f_value(1);
         try{
          f_value = Fisher_Dist::finv(p,m,n);
         }
         catch (int i)
         {
             if( i == -1)
                 qDebug() << "p value out of range";
             else if( i == -2)
                 qDebug() << "m or n are below 0";
                 
        }
        return f_value;
     }
     
     QList<int> InvertLockedList(const QList<int> &locked)
     {
         QList<int> lock;
         for(int i = 0; i < locked.size(); ++i)
             lock << !locked[i];
         return lock;
    }
    void ExportResults(const QString& filename, const QList<QJsonObject> &models)
    {
        QJsonObject toplevel;
        int i = 0;
        for(const QJsonObject &obj: qAsConst(models))
        {
            QJsonObject constants = obj["data"].toObject()["constants"].toObject();
            QStringList keys = constants.keys();
            bool valid = true;
            for(const QString &str : qAsConst(keys))
            {
                double var = constants[str].toString().toDouble();
                valid = valid && (var > 0);
            }
            toplevel["model_" + QString::number(i++)] = obj;       
        }
        JsonHandler::WriteJsonFile(toplevel, filename);
    }

}

namespace Print{
    
QString TextFromConfidence(const QJsonObject &result, const QPointer<AbstractModel> model)
{
    QString text, box_message("");
    qreal value = result["value"].toDouble();
    QString pot;
    QString nr;
    QString const_name;
    if(result["type"] == "Global Parameter")
    {
        nr = " = " + model->formatedGlobalParameter(value);
        pot = model->GlobalParameterPrefix();
    }else if(result["type"] == "Local Parameter")
    {
        nr = " = " + model->formatedLocalParameter(value);
        pot = model->LocalParameterPrefix(); 
    }
    QJsonObject confidence = result["confidence"].toObject();
    qreal upper = confidence["upper"].toDouble();
    qreal lower = confidence["lower"].toDouble();
    qreal conf = result["error"].toDouble();
    text = "<tr><td><b>" + result["name"].toString() + const_name + ":</b></td><td> <b>" + pot + QString::number(value) + " " + nr + " * " + pot + "(+ " + QString::number(upper-value, 'g', 3) + " / " + QString::number(lower-value, 'g', 3) + ") * </b></td></tr>\n";
    text += "<tr><td>"+QString::number(conf, 'f', 2) + "% Confidence Intervall=</td><td> <b>" +pot + QString::number(lower, 'f', 4) + " -" + pot + QString::number(upper, 'f', 4) + "</b></td></tr>\n"; 
    if(result.contains("boxplot"))
    {
        SupraFit::BoxWhisker box = ToolSet::Object2Whisker(result["boxplot"].toObject());
        text += "<tr><td>Median: </td><td> <b>" +QString::number(box.median, 'f', 4) + "</b></td></tr>\n"; 
        text += "<tr><td>Notches: </td><td> <b>" + QString::number(box.LowerNotch(), 'f', 4) +"-" + QString::number(box.UpperNotch(), 'f', 4) +"</b></td></tr>\n"; 
        if(value > box.UpperNotch() || value < box.LowerNotch())
        {
            box_message = "<tr><th colspan=2><font color='red'>Estimated value exceeds notch of BoxPlot!</font></th></tr>\n"; 
        }
    }
    text += "\n";
    text += "<tr><td></td></tr>\n";
    return box_message+text;    
}
}
