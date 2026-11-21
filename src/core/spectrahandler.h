/*
 * SpectraHandler - import and preparation of spectra series (UV-Vis titrations)
 * Copyright (C) 2020 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <cmath>

#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QPointF>
#include <QtCore/QVector>

#include <Eigen/Dense>

#include "src/core/trace/traceops.h"
#include "src/core/trace/traceset.h"

class DataTable;

typedef Eigen::VectorXd Vector;

class SpectraHandler : public QObject {
    Q_OBJECT
public:
    explicit SpectraHandler(QObject* parent = nullptr);
    virtual ~SpectraHandler();
    inline QStringList getOrder() const { return m_spectra.ids(); }

    void setSpectrafromFile(const QString& file);
    void addSpectrum(const QString& file);
    void addDirectory(const QString& dir, const QString& suffix);

    void clearFiles();
    /*! \brief Removes the spectrum with this id (raw data of the others unchanged); false if there is none. Claude Generated */
    bool removeSpectrum(const QString& id);

    inline QList<QPointF> Data(const QString& hash) const { return m_spectra.trace(hash).points(); }
    inline QString Name(const QString& hash) const { return m_spectra.trace(hash).name(); }
    inline QString Path(const QString& hash) const { return m_spectra.trace(hash).source(); }
    /*! \brief The raw spectra (ids = getOrder()). Claude Generated */
    inline const TraceSet& Spectra() const { return m_spectra; }

    /*! \brief Processing steps applied to every raw spectrum before CompileSimpleTable, SpectraMatrix
     * and VarCovarSelect use it; the raw spectra stay unchanged. Empty = the raw spectra. Claude Generated */
    inline const TraceOpList& Operations() const { return m_operations; }
    /*! \brief Replaces the processing steps (see Operations()). Claude Generated */
    inline void setOperations(const TraceOpList& operations) { m_operations = operations; }
    /*! \brief The spectra after Operations() (ids = getOrder()); a spectrum whose processing fails is
     * empty, *error names the first failure. Without steps a copy of Spectra(). Claude Generated */
    TraceSet ProcessedSpectra(QString* error = nullptr) const;

    inline void addXValue(double x) { m_x << x; }
    //inline void removeXValue()
    inline void setXValues(const QVector<double>& x) { m_x = x; }
    inline QVector<double> XValues() const { return m_x; }
    DataTable* CompileSimpleTable();

    /*! \brief Matrix of the spectra (rows) over their common x range, as used by VarCovarSelect.
     * Spectra of another sampling are interpolated onto the axis of the first one. Claude Generated */
    inline Eigen::MatrixXd SpectraMatrix() const { return PrepareMatrix(); }
    QVector<double> VarCovarSelect(int max_number, bool do_clustering = true, bool averaged = false);

    QJsonObject getSpectraData() const;
    void LoadData(const QJsonObject& data);

    inline double XMax() const { return x_max; }
    inline double XMin() const { return x_min; }
    inline double YMax() const { return y_max; }
    inline double YMin() const { return y_min; }

    void ParseData();

    void resetRange();
    void setXRange(double x_start, double x_end);
    /*! \brief Selected range of VarCovarSelect (stored as "XStart"/"XEnd"). Claude Generated */
    inline double XStart() const { return m_x_start; }
    inline double XEnd() const { return m_x_end; }

    /*! \brief Unit conversion of the intensity: CompileSimpleTable divides every intensity by it.
     * Default 1000 (the factor used before it was configurable); stored as "IntensityDivisor". Claude Generated */
    inline double IntensityDivisor() const { return m_intensity_divisor; }
    /*! \brief Sets the intensity divisor (see IntensityDivisor()); zero or non-finite values are ignored. Claude Generated */
    inline void setIntensityDivisor(double divisor)
    {
        if (divisor != 0 && std::isfinite(divisor))
            m_intensity_divisor = divisor;
    }
signals:
    void Updated();

private:
    TraceSet m_spectra; ///< raw spectra in titration order; name = file name, source = directory
    TraceOpList m_operations; ///< processing of every spectrum, stored as "TraceOps" when not empty
    QString m_table_file; ///< file of setSpectrafromFile (one spectrum per row), stored as "TableFile"
    QVector<double> m_x;
    DataTable* m_table = nullptr; ///< table of the last setSpectrafromFile, owned by the handler

    Eigen::MatrixXd PrepareMatrix() const;
    XYTrace MakeSpectrum(const Vector& x, const Vector& y, const QString& filename = QString()) const;
    XYTrace MakeSpectrum(const QPair<Vector, Vector>& spect, const QString& filename = QString()) const;

    Eigen::MatrixXd VarCovarMatrix() const;

    mutable QVector<double> m_x_ranges;
    double m_intensity_divisor = 1000.0;
    double x_min = 0, x_max = 0, y_min = 0, y_max = 0, m_x_start = 0, m_y_start = 0, m_x_end = 0, m_y_end = 0;
};
