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

#include <QtWidgets/QWidget>

#include <QtCharts/QChart>

typedef Eigen::VectorXd Vector;

class QComboBox;
class QLineEdit;
class QSpinBox;
class QPushButton;
class QTableWidget;

class LineSeries;
class ChartView;

class ThermogramWidget : public QWidget {
    Q_OBJECT

public:
    ThermogramWidget(QWidget* parent = nullptr);
    void setThermogram(PeakPick::spectrum* spec, qreal offset = 0);

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
    void IntegrationChanged();

public slots:

private:
    void setUi();
    void UpdateTable();
    void UpdatePlot();
    void UpdateLimits();
    void UpdateBase();

    void fromSpectrum(const PeakPick::spectrum* original, LineSeries* series);
    void fromPolynomial(const Vector& coeff, LineSeries* series);

    void Integrate(std::vector<PeakPick::Peak>* peaks, const PeakPick::spectrum* original);

    QComboBox *m_baseline_type, *m_fit_type;
    QSpinBox* m_degree;
    QLineEdit *m_constant, *m_stdev, *m_mult;
    QPushButton* m_fit_button;
    QCheckBox* m_limits;
    QTableWidget* m_table;
    ChartView* m_thermogram;
    QtCharts::QChart* m_data;
    LineSeries *m_thermogram_series, *m_baseline_series, *m_lower, *m_upper;
    PeakPick::spectrum m_spec;
    std::vector<PeakPick::Peak> m_peak_list;
    bool m_spectrum = false;
    QVector<qreal> m_peaks;
    Vector m_baseline;
    qreal m_scale = 4.184, m_offset = 0;
    QString m_base, m_fit;

private slots:
    void UpdateBaseLine(const QString& str);
    void UpdateFit(const QString& str);
    void FitBaseLine();
};
