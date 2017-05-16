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
#include <QtCore/QPointer>
#include <QtCore/QAbstractTableModel>

#include <random>

typedef Eigen::VectorXd Vector;

class DataTable : public QAbstractTableModel
{
    Q_OBJECT
public:
    DataTable(QObject *parent = 0);
    DataTable(int columns, int rows, QObject *parent);
    DataTable(Eigen::MatrixXd table, Eigen::MatrixXd checked_table);
    DataTable(DataTable *other);
    DataTable(DataTable &other);
    ~DataTable();
    int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    int columnCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    virtual Qt::ItemFlags flags(const QModelIndex& index) const Q_DECL_OVERRIDE;
    
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) Q_DECL_OVERRIDE;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    virtual bool setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role = Qt::EditRole) Q_DECL_OVERRIDE;
    qreal data(int column, int row) const;
    qreal & data(int column, int row);
    void CheckRow(int row);
    inline void setCheckedTable(Eigen::MatrixXd checked) { m_checked_table = checked; }
    inline Eigen::MatrixXd CheckedTable() const { return m_checked_table; }
    inline DataTable *BlockRows(int row_begin, int row_end) const { return Block(row_begin, 0, row_end, columnCount() -1); } 
    inline DataTable *BlockColumns(int column_begin, int column_end) const { return Block(0, column_begin, rowCount(), column_end); } 
    DataTable *Block(int row_begin, int column_begin, int row_end, int column_end) const;
    
    bool isChecked(int i, int j) const;
    void insertRow(const QVector<qreal> &row);
    void setRow(const QVector<qreal> &vector, int row);
    void setRow(const Vector &vector, int row);
    void setColumn(const QVector<qreal> &vector, int column);
    void setColumn(const Vector &vector, int column);
    
    Vector Row(int row);
    Vector Row(int row, const QList<int> &active);
    Vector firstRow(); 
    Vector lastRow(); 
    
    QVector<qreal> toList() const;
    
    void Debug() const ;
    inline QStringList header() const { return m_header; }
    inline void setCheckable(bool checkable) { m_checkable = checkable; }
    inline void setEditable(bool editable) { m_editable = editable; }
    inline bool Checkable() const { return m_checkable; }
    DataTable *PrepareMC(std::normal_distribution<double> &Phi, std::mt19937 &rng);
    DataTable *PrepareBootStrap(std::uniform_int_distribution<int> &Uni, std::mt19937 &rng, const QVector<qreal> &vector);
    QString ExportAsString() const;
    QStringList ExportAsStringList() const;
    
private:
    Eigen::MatrixXd m_table, m_checked_table;
    QStringList m_header;
    qreal m_empty;
    QReadWriteLock mutex;
    bool m_checkable, m_editable;
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
    int m_host_assignment;
    
    DataTable *m_dependent_model, *m_independent_model, *m_raw_data;
    QList<qreal > m_scaling;
    
    void check();
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
    

    
    inline void addPoint(QVector<qreal > conc, QVector<qreal > data)
    {
        d->m_independent_model->insertRow(conc);
        d->m_dependent_model->insertRow(data);
        if(conc.size() != d->m_scaling.size())
            for(int i = 0; i < d->m_independent_model->columnCount(); ++i)
                d->m_scaling << 1;
    }
    

    inline int Size() const { return DataPoints(); } 
    inline int IndependentVariableSize() const { return d->m_independent_model->columnCount(); }
    inline int DataPoints() const { return d->m_dependent_model->rowCount(); }
    inline int SeriesCount() const {return d->m_dependent_model->columnCount(); }
    inline int Type() const { return d->m_type;     }
    inline void setType(int type) { d->m_type = type; }
    inline DataTable * IndependentModel() { return d->m_independent_model; }
    inline DataTable * DependentModel() { return d->m_dependent_model; }
    inline DataTable * IndependentModel() const { return d->m_independent_model; }
    inline DataTable * DependentModel() const { return d->m_dependent_model; }
    inline void setIndependentTable(DataTable *table) 
    { 
        d->m_independent_model = table;     
        d->m_independent_model->setCheckable(false);
        if(d->m_independent_model->columnCount() != d->m_scaling.size())
            for(int i = 0; i < d->m_independent_model->columnCount(); ++i)
                d->m_scaling << 1;
    }
    inline void setDependentTable(DataTable *table) { d->m_dependent_model = table; d->m_dependent_model->setCheckable(true); }
    void SwitchConentrations();
    QList<qreal >  getSignals(QList<int > dealing_signals = QVector<int >(1,0).toList());
    qreal InitialHostConcentration(int i);
    qreal InitialGuestConcentration(int i);
    inline int HostAssignment() const { return d->m_host_assignment; }
 
    qreal XValue(int i) const;
    /* 
     * !\brief Export data to json
     */
    const QJsonObject ExportData(const QList<int> &active = QList<int>()) const;
    /*
     * !\brief Import data from json
     */
    bool ImportData(const QJsonObject &topjson);
    inline QList<qreal> getScaling() const { return d->m_scaling; }
    inline void setScaling(const QList<qreal> &scaling) { d->m_scaling = scaling; }
    void setHeader(const QStringList &strlist);
    void OverrideDependentTable(DataTable *table);
    
protected:
    QExplicitlySharedDataPointer<DataClassPrivate > d;
     
signals:
    void RowAdded();
    void ActiveSignalsChanged(QList<int > active_signals);
};

#endif // DATACLASS_H
