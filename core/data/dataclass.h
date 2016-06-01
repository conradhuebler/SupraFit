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

#ifndef DATACLASS_H
#define DATACLASS_H

#include <QtCore/QVector>
#include <QtCore/QMap>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QVXYModelMapper>
#include <QPointer>
#include <QColor>

class QStandardItemModel;

class DataPoint
{
    
public:
    
    DataPoint();
    DataPoint(const DataPoint &other);
    DataPoint(const DataPoint *other);
    DataPoint(qreal conc1, qreal conc2, QVector<qreal > data);
    ~DataPoint() {}
    qreal Conc1() const { return m_conc1; }
    void setConc1(qreal conc1)  { m_conc1 = conc1; }
    qreal Conc2() const { return m_conc2; }
    void setConc2(qreal conc2) { m_conc2 = conc2; }
    QVector<qreal> Data() const {  return m_data; }
    void AppendData(qreal p) { m_data << p; }
    void SetData(QVector<qreal > data) { m_data = data; }
private:
    qreal m_conc1, m_conc2;
    QVector<qreal > m_data;
};



class DataClass : public QObject
{
    Q_OBJECT
    public: 
        
    DataClass(QObject *parent = 0);
    DataClass(int type = 1, QObject *parent = 0);
    DataClass(const DataClass *other);
    DataClass(const DataClass &other);
    virtual ~DataClass();
    
    enum { 
        DiscretData = 1,
        ContiuousData = 2
    };
    enum PlotMode { H, G, HG, GH};
    inline void addPoint(qreal conc1, qreal conc2, QVector<qreal > data)
    {
        if(m_maxsize == 0)
            m_maxsize = data.size();
        else if(m_maxsize > data.size())
            m_maxsize = data.size();
        m_data << DataPoint(conc1, conc2, data);
    }
    
    inline void addPoint(DataPoint point)
    {
                
        if(m_maxsize == 0)
            m_maxsize = point.Data().size();
        else if(m_maxsize > point.Data().size())
            m_maxsize = point.Data().size();
        m_data << point;
    }
    
    inline void addPoint(DataPoint *point)
    {
        m_data << DataPoint(point);
    }   
    inline DataPoint* operator[](int i)
    {

            return &m_data[i];
    }
    inline const DataPoint* operator[](int i) const
    {
            return &m_data[i];
    }
    QColor color(int i) const;
    inline int Size() const { return m_maxsize; } 
    inline int DataPoints() const { return m_data.size(); }
    inline int Type() const { return m_type;     }
    inline void setType(int type) { m_type = type; }
    inline QStandardItemModel * ConcentrationModel() { return m_concentration_model; }
    inline QStandardItemModel * SignalModel() { return m_signal_model; }
    inline QPointer<QtCharts::QVXYModelMapper> DataMapper(int i) { return m_signal_mapper[i]; }
    void SwitchConentrations();
    inline bool* Concentration() const { return m_concentrations; }
    inline void setPlotMode(PlotMode mode)  {  m_plotmode = mode;  }
private:
    QStringList m_names;
    QVector<QColor > m_colors;
    QStandardItemModel *m_signal_model, *m_concentration_model;
    QVector<QPointer<QtCharts::QVXYModelMapper> >m_signal_mapper;
    void CreateItemModel();
    
    
protected:
    QVector< DataPoint> m_data;
    int m_type, m_maxsize;
    bool *m_concentrations;
    PlotMode m_plotmode;
    qreal XValue(int i) const;
};

#endif // DATACLASS_H
