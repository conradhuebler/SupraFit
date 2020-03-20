/*
 * <Class to handle thermogram import and manipulation.>
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

#include <Eigen/Dense>

#include <QtCore/QJsonObject>
#include <QtCore/QObject>
#include <QtCore/QPointF>
#include <QtCore/QVector>

#include "libpeakpick/analyse.h"
#include "libpeakpick/baseline.h"
#include "libpeakpick/peakpick.h"

class ThermogramHandler : public QObject {
    Q_OBJECT
public:
    ThermogramHandler();

    QVector<QPointF> Series();
    QVector<QPointF> Baseline();

    inline void setThermogram(const PeakPick::spectrum& spectrum) { m_spectrum = spectrum; }
    inline void setPeakList(const std::vector<PeakPick::Peak>& peak_list) { m_peak_list = QVector<PeakPick::Peak>::fromStdVector(peak_list); }
    inline void setPeakList(const QVector<PeakPick::Peak>& peak_list) { m_peak_list = peak_list; }

    void Initialise();

    inline const PeakPick::spectrum* Spectrum() const { return &m_spectrum; }

    inline const QVector<PeakPick::Peak>* Peaks() const { return &m_peak_list; }

    inline QVector<qreal> Integrals() const { return m_integrals_raw; }

    inline QVector<QPointF> PeakRules() const { return m_peak_rules; }

    inline void setPeakRules(const QVector<QPointF>& peak_rules) { m_peak_rules = peak_rules; }

    inline void setCalibrationStart(int start) { m_CalibrationStart = start; }

    inline void setThermogramBegin(qreal begin) { m_ThermogramBegin = begin; }
    inline qreal ThermogramBegin() const { return m_ThermogramBegin; }

    inline void setThermogramEnd(qreal end) { m_ThermogramEnd = end; }
    inline qreal ThermogramEnd() const { return m_ThermogramEnd; }

    inline qreal IntegrationRangeThreshold() const { return m_integration_range_threshold; }

    inline void setAveraged(bool averaged) { m_averaged = averaged; }
    inline qreal CalibrationRatio() const { return m_calibration_ratio; }

    void IntegrateThermogram();
    void CalibrateSystem();
    void AdjustIntegrationRange();

    void setThermogramParameter(const QJsonObject& thermogram_parameter) { m_thermogram_parameter = thermogram_parameter; }
    QJsonObject getThermogramParameter() const;

    void setCalibrationHeat(qreal heat) { m_CalibrationHeat = heat; }
    void setConstantOffset(qreal offset) { m_constant_offset = offset; }

    void UpdatePeaks();
    void FitBaseLine();

    qreal Calibration() const { return m_calibration_peak.integ_num; }

    inline QList<QPointF> ThermogramSeries() const { return m_thermogram_series; }
    inline QList<QPointF> BaselineSeries() const { return m_baseline_series; }
    inline QList<QPointF> BaselineGrid() const { return m_baseline_grid; }
    inline QList<QPointF> BaselineIgnored() const { return m_baseline_ignored_series; }

    void ConvertRules();
    void CompressRules();

    inline void setCurrentCutOption(const QString& current_integration_scheme) { m_current_integration_scheme = current_integration_scheme; }
    inline void setInitialThreshold(double initial_threshold) { m_initial_threshold = initial_threshold; }
    inline void setMaximalIterations(int iterations) { m_iterations = iterations; }
    inline void setOvershotCounter(int overshot_counter) { m_overshot_counter = overshot_counter; }
    inline void setCutBefore(bool cut_before) { m_cut_before = cut_before; }
    inline int LastIterations() const { return m_last_iteration_max; }

private:
    /* Chart Series use QList */
    QList<QPointF> m_thermogram_series, m_baseline_series, m_baseline_grid, m_baseline_ignored_series;
    QVector<QPointF> m_peak_rules;
    QVector<qreal> m_integrals_list, m_integrals_raw;
    QVector<PeakPick::Peak> m_peak_list;
    PeakPick::spectrum m_spectrum;
    PeakPick::Peak m_calibration_peak;
    PeakPick::BaseLineResult m_baseline;

    QJsonObject m_thermogram_parameter;

    int m_last_iteration_max = 0, m_iterations = 0, m_overshot_counter = 1, m_PeakCount = 0;
    qreal m_ThermogramBegin = 0, m_ThermogramEnd = 0, m_PeakDuration = 0, m_CalibrationStart = 0, m_constant_offset = 0, m_CalibrationHeat = 0, m_calibration_ratio = 0, m_scaling_factor = 1;
    qreal m_offset = 0, m_frequency = 1, m_initial_threshold = -1, m_integration_range_threshold = 0;
    bool m_initialised = false, m_cut_before = false, m_rules_imported = false, m_averaged = true;
    QString m_current_integration_scheme = QString("Custom"), m_last_integration_scheme;
    QStringList m_integration_scheme = QStringList() << "Custom"
                                                     << "Zero"
                                                     << "Threshold";

    void LegacyLoad();
    void LoadParameter();
    double ResizeIntegrationRange(double threshold, int direction);
    void ApplyThermogramIntegration();
    void UpdateBaseLine();

signals:
    void ThermogramChanged();
    void BaseLineChanged();
    void CalibrationChanged();
    void PeakRulesChanged();
    void Message(const QString& message);
};
