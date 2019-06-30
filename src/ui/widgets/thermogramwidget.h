/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2018 - 2019 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <QtCore/QRunnable>

#include <QtWidgets/QWidget>

#include <QtCharts/QChart>

#include "libpeakpick/baseline.h"

class QComboBox;
class QLineEdit;
class QSpinBox;
class QSplitter;
class QRadioButton;
class QPushButton;
class QTableWidget;
class QTableWidgetItem;
class QPlainTextEdit;

class ChartView;
class LineSeries;
class ScatterSeries;

/*
class BaseLineWorker : public QRunnable
{
public:
    inline BaseLineWorker(){ setAutoDelete(false); }
    virtual void run() override();
    void setSpectrum();
    void setBaseLineRange();
    void setNoCoeffs(int coeffs);
private:
};
*/
class ThermogramWidget : public QWidget {
    Q_OBJECT

public:
    enum FileType {
        ITC = 0,
        RAW = 1
    };

    ThermogramWidget(QWidget* parent = nullptr);
    ~ThermogramWidget();

    void setThermogram(PeakPick::spectrum* spec, qreal offset = 0.0);

    void setPeakList(const std::vector<PeakPick::Peak>& peak_list);
    void Update();

    // inline QVector<qreal> PeakList() const { return m_peaks; }

    inline void setOffset(qreal offset) { m_offset = offset; }
    void PickPeaks();
    void clear();

    void setFit(const QJsonObject& fit);
    QJsonObject Fit() const;

    inline std::vector<PeakPick::Peak> Peaks() const { return m_peak_list; }

    inline void setFileType(const FileType filetype) { m_filetype = filetype; }
signals:
    void IntegrationChanged();
    void PeaksChanged();
    void Offset();

public slots:
    inline void setFrequency(qreal frequency) { m_frequency = frequency; }
    void FitBaseLine();

private:
    void setUi();
    void UpdateTable();
    void UpdatePlot();
    void UpdateLimits();
    void CreateSeries();

    void fromSpectrum(const PeakPick::spectrum* original, LineSeries* series);
    void fromPolynomial(const Vector& coeff, LineSeries* series);

    void Integrate(std::vector<PeakPick::Peak>* peaks, const PeakPick::spectrum* original);
    void CalibrateSystem();
    void ApplyCalibration();

    QComboBox *m_baseline_type, *m_fit_type;
    QSpinBox *m_coeffs, *m_filter, *m_peak_box, *m_peak_count, *m_peak_sensitivity;
    QDoubleSpinBox *m_peaks_start, *m_peaks_end, *m_peaks_time, *m_const_offset, *m_calibration_start, *m_calibration_heat;
    QLineEdit *m_constant, *m_stdev, *m_mult;
    QRadioButton *m_peak_wise, *m_full_spec;
    QPushButton *m_fit_button, *m_peak_apply, *m_get_peaks_start, *m_get_peaks_end, *m_get_peaks_range, *m_auto_pick;
    QCheckBox *m_limits, *m_smooth, *m_poly_slow;
    QTableWidget* m_table;
    ChartView* m_thermogram;
    //QtCharts::QChart* m_data;
    QPlainTextEdit* m_baseline_polynom;
    QLabel* m_calibration_label;
    QSplitter* m_splitter;
    LineSeries *m_thermogram_series, *m_baseline_series, *m_lower, *m_upper, *m_left, *m_right, *m_calibration_line;
    ScatterSeries *m_base_grids, *m_calibration_grid;

    QVector<qreal> m_integrals_raw;
    PeakPick::spectrum m_spec;
    std::vector<PeakPick::Peak> m_peak_list;
    PeakPick::Peak m_calibration_peak;
    bool m_spectrum = false, m_block = false;

    /* 1 - get start
     * 2 - get end */
    int m_get_time_from_thermogram = 0;

    PeakPick::BaseLineResult m_baseline;
    Vector m_initial_baseline = Vector(0);
    qreal m_offset = 0, m_frequency = 1;
    QString m_base, m_fit;
    FileType m_filetype;

private slots:
    void UpdateBaseLine(const QString& str);
    void UpdateFit(const QString& str);
    void PeakDoubleClicked(const QModelIndex& index);
    void PeakDoubleClicked(int peak);
    void PeakChanged(int row, int column, int value);
    void UpdatePeaks();
    void AddRectanglePeak(const QPointF& point1, const QPointF& point2);
    void PointDoubleClicked(const QPointF& point);
    void Divide2Peaks();
};
