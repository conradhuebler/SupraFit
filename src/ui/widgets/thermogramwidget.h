/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2018 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <QtWidgets/QTableWidgetItem>
#include <QtWidgets/QWidget>

#include <QtCharts/QChart>

#include "src/core/thermogramhandler.h"

#include "libpeakpick/baseline.h"

class QComboBox;
class QCheckBox;
class QDoubleSpinBox;
class QLabel;
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

class PeakRule : public QTableWidgetItem {
public:
    PeakRule() = default;

    PeakRule(const QString& str)
        : QTableWidgetItem(str)
    {
    }

    bool operator<(const QTableWidgetItem& other) const override
    {
        return (this->text().toDouble() < other.text().toDouble());
    }
};

class ThermogramWidget : public QWidget {
    Q_OBJECT

public:
    enum FileType {
        ITC = 0,
        RAW = 1
    };

    ThermogramWidget(QPointer<ThermogramHandler> thermogram, QWidget* parent = nullptr);
    ~ThermogramWidget();

    void Update();

    void clear();

    inline void setFileType(const FileType filetype) { m_filetype = filetype; }

    void addOptionalSeries(const QList<QPointF>& series, const QString& name);

public slots:
    void UpdatePeaks();
    void UpdateBaseLine();

    /*! \brief Show `factor` in the cal->J combo without pushing it anywhere.
     *
     * The factor is owned by the ItcProcessor - experiment and dilution must share it - so this
     * widget is only a view on it. Blocks the combo's own signal, so syncing the two widgets cannot
     * feed back. Claude Generated */
    void setScalingFactor(qreal factor);

signals:
    //! The user picked a new cal->J factor; the owner (ItcProcessor) decides what to do. Claude Generated
    void ScalingFactorChanged(qreal factor);

private:
    void LoadDefault();
    void setUi();
    /*! \brief Parts of setUi(), in the order they run (Claude Generated, WP5). */
    //! Guide label and thermogram chart with its peak/zoom/scale interactions.
    void setupChart();
    //! Peak list and peak rule tables with the rule buttons, next to the chart in a splitter.
    QLayout* setupPeakLists();
    //! Spin boxes, combos and buttons of the parameter panel and their connections.
    void setupControls();
    //! Group boxes of the parameter panel (start/end/inject time, peak ranges, calibration, scaling, advanced).
    QLayout* setupParameterPanel();
    //! Values from QSettings ("thermogram" group) and the first peak rule.
    void restoreSettings();
    //! Connections to the stored settings and to the handler's signals.
    void connectControls();
    void UpdateTable();
    void InitialiseChart();
    void CreateSeries();
    void UpdateSeries();
    void ApplyParameter();

    void ResetGuideLabel();

    void setGuideText(const QString& str);

    QComboBox *m_integration_range, *m_scaling;
    QSpinBox *m_iterations, *m_overshot;

    QDoubleSpinBox *m_peaks_start, *m_peaks_end, *m_peaks_time, *m_const_offset, *m_calibration_start, *m_calibration_heat, *m_integration_range_threshold, *m_gradient;
    QPushButton *m_peak_apply, *m_get_peaks_start, *m_get_peaks_end, *m_convert_rules, *m_load_rules, *m_write_rules, *m_clear_rules, *m_get_calibration_start;
    QCheckBox *m_direction, *m_averaged;
    QTableWidget *m_table, *m_peak_rule_list;
    ChartView* m_thermogram;

    QLabel* m_guide_label;
    QSplitter* m_splitter;
    LineSeries *m_thermogram_series, *m_lower, *m_upper, *m_peak_start_line, *m_peak_end_line, *m_optional_series;
    ScatterSeries *m_base_grids, *m_baseline_series, *m_baseline_ignored_series;

    QVector<PeakPick::Peak> m_peak_list;
    bool m_spectrum = false, m_peak_edit_mode = false, m_rules_imported = false;

    /* 1 - get start
     * 2 - get end */
    int m_get_time_from_thermogram = 0, m_current_peaks_rule = 0, m_current_peak = 0, m_calibration_start_int = 0;

    qreal m_CalibrationStart = 0, m_ScalingFactor = 0;
    FileType m_filetype = ITC;
    QStringList m_Peak_Cut_Options = QStringList() << "Custom"
                                                   << "Zero"
                                                   << "Threshold";

    QPointer<ThermogramHandler> m_stored_thermogram;

private slots:
    void PeakDoubleClicked(const QModelIndex& index);
    void PeakRuleDoubleClicked(const QModelIndex& index);
    void PeakRuleDoubleClicked(const QTableWidgetItem* item, int peak);

    void PeakDoubleClicked(int peak);
    void AddRectanglePeak(const QPointF& point1, const QPointF& point2);
    void PointDoubleClicked(const QPointF& point);
    void scaleUp();
    void scaleDown();
    void UpdateRules();
    void LoadRules();
    void WriteRules();
};
