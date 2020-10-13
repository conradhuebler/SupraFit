/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2020 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QPointF>
#include <QtCore/QVector>

#include "libpeakpick/analyse.h"
#include "libpeakpick/baseline.h"
#include "libpeakpick/peakpick.h"

class DataTable;

struct Spectrum {
    PeakPick::spectrum m_spectrum;
    QList<QPointF> m_xy;
    QString m_filename;
    QString m_path;
};

class SpectraHandler : public QObject {
    Q_OBJECT
public:
    explicit SpectraHandler(QObject* parent = nullptr);
    virtual ~SpectraHandler() {}
    inline QStringList getOrder() const { return m_order; }

    void addSpectrum(const QString& file);
    void addDirectory(const QString& dir, const QString& suffix);

    inline QList<QPointF> Data(const QString& hash) { return m_spectra.value(hash).m_xy; }
    inline QString Name(const QString& hash) { return m_spectra.value(hash).m_filename; }
    inline QString Path(const QString& hash) { return m_spectra.value(hash).m_path; }

    inline void addXValue(double x) { m_x << x; }
    //inline void removeXValue()
    inline void setXValues(const QVector<double>& x) { m_x = x; }
    inline QVector<double> XValues() const { return m_x; }
    DataTable* CompileSimpleTable();

    void PCA();
    QVector<double> VarCovarSelect(int number);

    QJsonObject getSpectraData() const;
    void LoadData(const QJsonObject& data);

    inline double XMax() const { return x_max; }
    inline double XMin() const { return x_min; }
    inline double YMax() const { return y_max; }
    inline double YMin() const { return y_min; }

    void ParseData();

signals:
    void Updated();

private:
    QHash<QString, Spectrum> m_spectra;
    QStringList m_order;
    QVector<double> m_x;

    Eigen::MatrixXd PrepareMatrix() const;
    Spectrum MakeSpectrum(const Vector& x, const Vector& y, const QString& filename = QString());
    Spectrum MakeSpectrum(const QPair<Vector, Vector>& spect, const QString& filename = QString());

    Eigen::MatrixXd VarCovarMatrix() const;

    mutable QVector<double> m_x_ranges;
    double x_min = 0, x_max = 0, y_min = 0, y_max = 0;
};
