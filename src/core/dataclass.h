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

#include <Eigen/Dense>

#include <QtCore/QReadWriteLock>
#include <QtCore/QMutexLocker>
#include <QtCore/QSharedData>
#include <QtCore/QVector>
#include <QtCore/QMap>
#include <QPointer>
#include <QDebug>
#include <QAbstractTableModel>

class DataTable : public QAbstractTableModel
{
public:
    DataTable(QObject *parent = 0);
    DataTable(int columns, int rows, QObject *parent = 0);
    DataTable(DataTable *other);
    DataTable(DataTable &other);
    ~DataTable();
    int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    int columnCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    virtual Qt::ItemFlags flags(const QModelIndex& index) const Q_DECL_OVERRIDE;
    
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
//     QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    qreal data(int column, int row) const;
    qreal & data(int column, int row);
    
    void insertRow(QVector<qreal> row);
//     void insertColumn(QVector<qreal> column);
    void setRow(QVector<qreal> vector, int row);
    void setColumn(QVector<qreal> vector, int column);
    
//     QVector<qreal> Row(int row);
    QList<qreal> Row(int row);
//     QVector<qreal> Column(int column);
//     QVector<qreal> firstRow(); 
    QList<qreal> firstRow(); 
//     inline QVector<qreal> firstColumn() { return Column( 0 ); }
//     QVector<qreal> lastRow(); 
    QList<qreal> lastRow(); 
//     inline QVector<qreal> lastColumn() { return Column(columnCount() -1 );}
    
    void Debug() const ;
private:
    Eigen::MatrixXd m_table;
    qreal m_empty;
    QReadWriteLock mutex;
};


class DataClassPrivate : public QSharedData
{
    
public:
    DataClassPrivate();
    DataClassPrivate(int i);
    DataClassPrivate(const DataClassPrivate *other);
    DataClassPrivate(const DataClassPrivate &other);
    ~DataClassPrivate();
    
    /*
     * Here are the datas
     */
    
    QStringList m_names;
    
    int m_type, m_maxsize;
    bool *m_concentrations;
    
    
    DataTable *m_signal_model, *m_concentration_model, *m_raw_data;
};


class DataClass : public QObject
{
    Q_OBJECT
    public: 
        
    DataClass(QObject *parent = 0);
    DataClass(const QJsonObject &json,int type = 1,  QObject *parent = 0);
    DataClass(int type = 1, QObject *parent = 0);
    DataClass(const DataClass *other);
    DataClass(const DataClass &other);
    virtual ~DataClass();
    
    enum { 
        DiscretData = 1,
        ContiuousData = 2,
        EmptyData = 3
    };
    
    enum PlotMode { 
        H = 1, 
        G = 2, 
        HG = 3, 
        GH = 4
    };
    
    inline void addPoint(QVector<qreal > conc, QVector<qreal > data)
    {
        d->m_concentration_model->insertRow(conc);
        d->m_signal_model->insertRow(data);
    }
    

    inline int Size() const { return DataPoints(); } 
    inline int Concentrations() const { return d->m_concentration_model->columnCount(); }
    inline int DataPoints() const { return d->m_signal_model->rowCount(); }
    inline int SignalCount() const {return d->m_signal_model->columnCount(); }
    inline int Type() const { return d->m_type;     }
    inline void setType(int type) { d->m_type = type; }
    inline DataTable * ConcentrationModel() { return d->m_concentration_model; }
    inline DataTable * SignalModel() { return d->m_signal_model; }
    inline DataTable * ConcentrationModel() const { return d->m_concentration_model; }
    inline DataTable * SignalModel() const { return d->m_signal_model; }
    void SwitchConentrations();
    inline bool* Concentration() const { return d->m_concentrations; }
    inline void setPlotMode(PlotMode mode)  {  m_plotmode = mode;  }
    QList<qreal >  getSignals(QList<int > dealing_signals = QVector<int >(1,0).toList());


    /*
    void setData(qreal point, int line, int row)
    {
        if(m_data.size() > line)
            m_data[line].setData(point, row);
    }
    
    inline void addRow(int row) 
    {
        for(int i = 0; i < m_data.size(); ++i)
        {
            m_data[i].setData(0, row);
            qDebug() << m_data[i][row];      
        }
        m_maxsize++;
  
        emit RowAdded();
    }*/  
    qreal XValue(int i) const;
    const QJsonObject ExportJSON() const;
    bool ImportJSON(const QJsonObject &topjson);

protected:
    PlotMode m_plotmode;
    QExplicitlySharedDataPointer<DataClassPrivate > d;
     
signals:
    void RowAdded();
    void ActiveSignalsChanged(QList<int > active_signals);
};

#endif // DATACLASS_H
