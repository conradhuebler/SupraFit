/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2022  Conrad Hübler <Conrad.Huebler@gmx.net>
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

class DataTable : public QAbstractTableModel {
    Q_OBJECT

public:
    DataTable(QObject* parent = 0);
    /*!\brief Construct new tabke with int rows and int columns
     */
    DataTable(int rows, int columns, QObject* parent);
    DataTable(Eigen::MatrixXd table, Eigen::MatrixXd checked_table, const QStringList& header = QStringList());
    DataTable(Eigen::MatrixXd table);

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
    void prepend(const QPointer<DataTable> table);

    void insertRow(const Vector& row, const Vector& checked);
    void insertRow(const Vector& row, bool zero = false);

    void insertRow(const QVector<qreal>& row, bool zero = false);
    void setRow(const QVector<qreal>& vector, int row);
    void setRow(const Vector& vector, int row);
    void setColumn(const QVector<qreal>& vector, int column);
    void setColumn(const Vector& vector, int column);

    void appendColumns(const DataTable& columns, bool keep_header = true);
    void prependColumns(const DataTable& columns, bool keep_header = true);

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
