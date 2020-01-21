/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016 - 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#pragma once

#include "src/global.h"

#include <Eigen/Dense>

#include <QtCore/QAbstractTableModel>
#include <QtCore/QJsonObject>
#include <QtCore/QMap>
#include <QtCore/QMutexLocker>
#include <QtCore/QPointer>
#include <QtCore/QReadWriteLock>
#include <QtCore/QSharedData>
#include <QtCore/QVector>

#include <random>

typedef Eigen::VectorXd Vector;

class DataClass;

class SystemParameter {
public:
    enum Type {
        String = 0,
        Scalar = 1,
        Boolean = 2,
        List = 3
    };

    inline SystemParameter(int index, const QString& name, const QString& description, Type type)
        : m_index(index)
        , m_name(name)
        , m_description(description)
        , m_type(type)
        , m_list(QStringList())
    {
    }
    inline SystemParameter() {}
    inline ~SystemParameter() {}

    inline QVariant value() const { return m_value; }
    inline QString Name() const { return m_name; }
    inline qreal Double() const { return m_value.toDouble(); }
    inline bool Bool() const { return m_value.toBool(); }
    inline QString getString() const { return m_value.toString(); }
    inline QStringList getList() const { return m_list; }
    inline QString Description() const { return m_description; }
    inline void setValue(const QVariant& value) { m_value = value; }
    inline bool isBool() const { return m_type == 2; }
    inline bool isString() const { return m_type == 0; }
    inline bool isScalar() const { return m_type == 1; }
    inline bool isList() const { return m_type == 3; }
    inline int Index() const { return m_index; }

    inline void setList(const QStringList& list) { m_list = list; }

private:
    Type m_type;
    QString m_name, m_description;
    QVariant m_value;
    QStringList m_list;
    int m_index;
};

class DataTable : public QAbstractTableModel {
    Q_OBJECT
public:
    DataTable(QObject* parent = 0);
    DataTable(int columns, int rows, QObject* parent);
    DataTable(Eigen::MatrixXd table, Eigen::MatrixXd checked_table);
    DataTable(DataTable* other);
    DataTable(DataTable& other);

    DataTable(const QJsonObject& table);

    virtual ~DataTable();

    bool isValid() const;
    void clear(int columns = 0, int rows = 0);

    int rowCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;
    int columnCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;
    virtual Qt::ItemFlags flags(const QModelIndex& index) const Q_DECL_OVERRIDE;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    virtual bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) Q_DECL_OVERRIDE;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    virtual bool setHeaderData(int section, Qt::Orientation orientation, const QVariant& value, int role = Qt::EditRole) Q_DECL_OVERRIDE;
    qreal data(int column, int row) const;
    qreal& data(int column, int row);

    qreal& operator[](int column);
    qreal& operator()(int column, int row);

    void CheckRow(int row);
    Vector DisableRow(int row);
    void PrintCheckedRows() const;
    void EnableAllRows();
    void setCheckedAll(bool checked);
    inline void setCheckedTable(Eigen::MatrixXd checked) { m_checked_table = checked; }
    inline Eigen::MatrixXd CheckedTable() const { return m_checked_table; }
    inline DataTable* BlockRows(int row_begin, int row_end) const { return Block(row_begin, 1, row_end, columnCount()); }
    inline DataTable* BlockColumns(int column_begin, int column_end) const { return Block(0, column_begin, rowCount(), column_end); }
    QPointer<DataTable> Block(int row_begin, int column_begin, int row_end, int column_end) const;

    bool isChecked(int i, int j) const;
    int isRowChecked(int i) const;
    int EnabledRows() const;

    void setChecked(int i, int j, bool checked);

    void append(const QPointer<DataTable> table);

    void insertRow(const Vector& row, const Vector& checked);
    void insertRow(const QVector<qreal>& row, bool zero = false);
    void setRow(const QVector<qreal>& vector, int row);
    void setRow(const Vector& vector, int row);
    void setColumn(const QVector<qreal>& vector, int column);
    void setColumn(const Vector& vector, int column);

    Vector Column(int row) const;

    Vector Row(int row) const;
    Vector Row(int row, const QList<int>& active) const;
    Vector firstRow();
    Vector lastRow();

    Vector CheckedRow(int row) const;

    QVector<qreal> toVector() const;
    QList<qreal> toList() const;

    void Debug(const QString& str = "None") const;
    inline QStringList header() const { return m_header; }
    inline void setHeader(const QStringList& header) { m_header = header; }
    inline void setCheckable(bool checkable) { m_checkable = checkable; }
    inline void setEditable(bool editable) { m_editable = editable; }
    inline bool isEditable() const { return m_editable; }
    inline bool Checkable() const { return m_checkable; }
    QPointer<DataTable> PrepareMC(std::normal_distribution<double>& Phi, std::mt19937& rng, QVector<int> cols = QVector<int>());
    QPointer<DataTable> PrepareMC(QVector<double> stddev, std::mt19937& rng, QVector<int> cols = QVector<int>());

    QPointer<DataTable> PrepareBootStrap(std::uniform_int_distribution<int>& Uni, std::mt19937& rng, const QVector<qreal>& vector);
    QString ExportAsString() const;
    QStringList ExportAsStringList() const;

    QJsonObject ExportTable(bool checked, const QVector<int> checked_table = QVector<int>()) const;
    bool ImportTable(const QJsonObject& table);

    inline Eigen::MatrixXd& Table() { return m_table; }

private:
    Eigen::MatrixXd m_table, m_checked_table;
    QStringList m_header;
    qreal m_empty;
    QReadWriteLock mutex;
    QMutex m_lock;

    bool m_checkable, m_editable;
};

class DataClassPrivateObject : public QObject {
    Q_OBJECT
public:
    DataClassPrivateObject() {}

signals:
    void SystemParameterChanged();
    void SystemParameterLoaded();
    void Update();
    void Message(const QString& str, int priority = 3);
    void Warning(const QString& str, int priority = 1);
};

class DataClassPrivate : public QSharedData {

public:
    enum DataType {
        Table = 1,
        Thermogram = 2,
        Spectrum = 3,
        Simulation = 10
    };

    DataClassPrivate();
    DataClassPrivate(int i);
    DataClassPrivate(const DataClassPrivate* other);
    DataClassPrivate(const DataClassPrivate& other);
    ~DataClassPrivate();

    void printReferenz() const;
    /*
     * Here are the datas
     */

    QStringList m_names;

    int m_maxsize;
    int m_host_assignment;
    int m_ref_counter = 1;
    QPointer<DataTable> m_dependent_model, m_independent_model;
    DataType m_datatype;
    QJsonObject m_raw_data;
    QJsonObject m_systemObject;

    QList<qreal> m_scaling;
    QMap<int, SystemParameter> m_system_parameter;
    QPointer<DataClassPrivateObject> m_info;
    QVector<QPointer<DataClass>> m_children;

    QString m_title, m_uuid, m_content, m_root_dir;
    void check();
};

class DataClass : public QObject {

    Q_OBJECT

public:
    DataClass(QObject* parent = 0);
    DataClass(const QJsonObject& json, int type = 1, QObject* parent = 0);
    DataClass(const DataClass* other);
    DataClass(const DataClass& other);
    virtual ~DataClass();

    virtual SupraFit::Model SFModel() const { return SupraFit::Data; }

    inline void addPoint(QVector<qreal> conc, QVector<qreal> data)
    {
        d->m_independent_model->insertRow(conc);
        d->m_dependent_model->insertRow(data);
        if (conc.size() != d->m_scaling.size())
            for (int i = 0; i < d->m_independent_model->columnCount(); ++i)
                d->m_scaling << 1;
    }

    inline QString UUID() const { return d->m_uuid; }

    void NewUUID();

    virtual inline int Size() const { return DataPoints(); }
    virtual inline int IndependentVariableSize() const { return d->m_independent_model->columnCount(); }
    virtual inline int DataPoints() const { return d->m_dependent_model->rowCount(); }
    virtual inline int SeriesCount() const { return d->m_dependent_model->columnCount(); }
    inline int Type() const { return d->m_datatype; }
    inline void setType(DataClassPrivate::DataType type) { d->m_datatype = type; }
    virtual inline DataTable* IndependentModel() { return d->m_independent_model; }
    virtual inline DataTable* DependentModel() { return d->m_dependent_model; }
    virtual inline DataTable* IndependentModel() const { return d->m_independent_model; }
    virtual inline DataTable* DependentModel() const { return d->m_dependent_model; }

    inline bool isSimulation() const { return Type() == DataClassPrivate::DataType::Simulation; }

    /*! \brief return text of stored data
     */
    QString Data2Text() const;

    /*! \brief reimplment, if more model specfic raw data information should be printed out
     */
    virtual QString Data2Text_Private() const { return QString(); }

    inline void setIndependentTable(DataTable* table)
    {
        d->m_independent_model = table;
        d->m_independent_model->setCheckable(false);
        if (d->m_independent_model->columnCount() != d->m_scaling.size())
            for (int i = 0; i < d->m_independent_model->columnCount(); ++i)
                d->m_scaling << 1;
    }

    DataClassPrivateObject* Info() const { return d->m_info; }

    inline void Updated() const { emit Info()->Update(); }

    inline void setDependentTable(DataTable* table)
    {
        d->m_dependent_model = table;
        d->m_dependent_model->setCheckable(true);
        DependentModelOverride();
    }
    void SwitchConentrations();
    virtual QList<qreal> getSignals(QList<int> dealing_signals = QVector<int>(1, 0).toList());

    inline int HostAssignment() const { return d->m_host_assignment; }

    inline qreal XValue(int i) const { return PrintOutIndependent(i); }
    /*
     * !\brief Export data to json
     */
    const QJsonObject ExportData() const;
    /*
     * !\brief Import data from json
     */
    bool ImportData(const QJsonObject& topjson, bool forceUUID = true);

    /*
     * !\brief Import data from json older files
     */
    bool LegacyImportData(const QJsonObject& topjson, bool forceUUID = true);

    inline QList<qreal> getScaling() const { return d->m_scaling; }
    inline void setScaling(const QList<qreal>& scaling) { d->m_scaling = scaling; }
    void setHeader(const QStringList& strlist);

    virtual void OverrideInDependentTable(DataTable* table);
    virtual void IndependentModelOverride() {}

    virtual void OverrideDependentTable(DataTable* table);

    virtual void DependentModelOverride() {}

    virtual void CheckedModelOverride() {}
    void OverrideCheckedTable(DataTable* table);

    /*! \brief Add a system parameter to the current model
     */
    void addSystemParameter(int index, const QString& str, const QString& description, SystemParameter::Type type);

    /*! \brief Get the SystemParameter with the specified index
     */
    SystemParameter getSystemParameter(int index) const;

    /*! \brief get a list of system parameters
     */
    QList<int> getSystemParameterList() const;

    /*! \brief Set the value of the system parameter
     */
    void setSystemParameterValue(int index, const QVariant& value);

    /*! \brief Set the value of the system parameter list
     */
    void setSystemParameterList(int index, const QStringList& value);

    /*! \brief set a systemparameter to the given one
     */
    void setSystemParameter(const SystemParameter& parameter);

    /*! \brief Overrides system parameter
     */
    inline void OverrideSystemParameter(const QMap<int, SystemParameter>& system_parameter)
    {
        d->m_system_parameter = system_parameter;
        emit Info()->SystemParameterChanged();
    }

    /*! \brief load previously cached system parameter
     */
    void LoadSystemParameter();

    /*! \brief write system parameterto json internal
     */
    void WriteSystemParameter();

    inline QMap<int, SystemParameter> SysPar() const { return d->m_system_parameter; }

    inline void RemoveSystemParameter(int key) { d->m_system_parameter.remove(key); }

    inline void detach() { d.detach(); }

    /*! \brief model dependented printout of the independant parameter
     */
    virtual qreal PrintOutIndependent(int i) const
    {
        if (!m_plot_x)
            return i + 1;
        else
            return d->m_independent_model->data(0, i);
    }

    inline void setPlotMode(bool plot_x) { m_plot_x = plot_x; }

    inline int PlotMode() const { return m_plot_x; }

    inline void setRawData(const QJsonObject& data) { d->m_raw_data = data; }

    inline void setDataType(DataClassPrivate::DataType type) { d->m_datatype = type; }

    inline DataClassPrivate::DataType DataType() const { return d->m_datatype; }

    inline void setProjectTitle(const QString& str)
    {
        d->m_title = str;
        emit ProjectTitleChanged(str);
    }

    inline QString ProjectTitle() const { return d->m_title; }

    /*! \brief override systemparameter with QJsonObject */
    void setSystemObject(const QJsonObject& object);

    /*! \brief return systemparameter as QJsonObject */
    inline QJsonObject getSystemObject() const { return d->m_systemObject; }

    QJsonObject ExportChildren(bool statistics = true, bool locked = false);

    inline virtual int ChildrenSize() const { return d->m_children.size(); }

    virtual inline QPointer<DataClass> Children(int i) { return d->m_children[i]; }

    /*! \brief Define the x axis label for charts
     */
    virtual QString XLabel() const { return QString("X"); }

    /*! \brief Define the y axis for charts
     */
    virtual QString YLabel() const { return QString("Y"); }

    inline QString Content() const { return d->m_content; }

    inline void setContent(const QString& str) { d->m_content = str; }

    inline void setRootDir(const QString& str) { d->m_root_dir = str; }

    inline QString RootDir() const { return d->m_root_dir; }

private:
    QMutex m_lock;
    bool m_plot_x = false;

protected:
    QExplicitlySharedDataPointer<DataClassPrivate> d;
    void AddChildren(QPointer<DataClass> children);

signals:
    void RowAdded();
    void ActiveSignalsChanged(QList<int> active_signals);
    void SystemParameterLoaded();
    void SystemParameterChanged();
    void ProjectTitleChanged(const QString& name);
    void Update(); //FIXME remove
    void Deleted();
    void Message(const QString& str, int priority);
    void Warning(const QString& str, int priority);
};
