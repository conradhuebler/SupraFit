/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <QWidget>

#include <QtCharts/QChart>

typedef Eigen::VectorXd Vector;

class QTableWidget;
class LineSeries;
class ChartView;

class ThermogramWidget : public QWidget {
    Q_OBJECT

public:
    ThermogramWidget(QWidget* parent = nullptr);
    inline void setThermogram(PeakPick::spectrum* spec)
    {
        m_spec = spec;
        m_spectrum = true;
        UpdatePlot();
    }
    inline void setPeakList(const std::vector<PeakPick::Peak>& peak_list)
    {
        m_peak_list = peak_list;
        UpdateTable();
    }
    void Update();
    inline QVector<qreal> PeakList() const { return m_peaks; }
    inline void setScale(qreal scale) { m_scale = scale; }
    inline void setOffset(qreal offset) { m_offset = offset; }
    void PickPeaks();
    void clear();

signals:

public slots:

private:
    void setUi();
    void UpdateTable();
    void UpdatePlot();

    LineSeries* fromSpectrum(const PeakPick::spectrum* original);
    void Integrate(std::vector<PeakPick::Peak>* peaks, const PeakPick::spectrum* original);

    QTableWidget* m_table;
    ChartView* m_thermogram;
    QtCharts::QChart* m_data;
    LineSeries *m_thermogram_series, *m_baseline_series, *m_lower, *m_upper;
    const PeakPick::spectrum* m_spec;
    std::vector<PeakPick::Peak> m_peak_list;
    bool m_spectrum = false;
    QVector<qreal> m_peaks;
    Vector m_baseline;
    qreal m_scale = 4.184, m_offset = 0;
};
