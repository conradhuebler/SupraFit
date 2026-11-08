/*
 * <Class to handle thermogram import and manipulation.>
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

#include <Eigen/Dense>

#include <QtCore/QJsonObject>
#include <QtCore/QObject>
#include <QtCore/QPointF>
#include <QtCore/QVector>

#include <optional>

#include "src/core/trace/xytrace.h"

#include "libpeakpick/analyse.h"
#include "libpeakpick/baseline.h"
#include "libpeakpick/peakpick.h"

/*! \brief One ITC thermogram (experiment or dilution): peaks, baseline, integrals, calibration.
 *
 * The raw curve is held as an `XYTrace` (WP5 of roadmap/input_data_editing.md); the libpeakpick
 * algorithms (peak ranges, baseline fit, numerical integration) run on a `PeakPick::spectrum` built
 * from it whenever the trace is set, so their numbers are those of the trace bit for bit. The chart
 * series are not stored here: ThermogramSeries()/Baseline*() compute them from the current state.
 * The libpeakpick maths itself (peak end convention etc.) is Track B and deliberately untouched. */
class ThermogramHandler : public QObject {
    Q_OBJECT
public:
    ThermogramHandler();

    /*! \brief Set the raw thermogram (time, heat flow). Claude Generated (WP5) */
    void setThermogram(const XYTrace& trace);
    //! Same, from a libpeakpick spectrum (loaders, Python bindings); only its x and y are taken.
    void setThermogram(const PeakPick::spectrum& spectrum);
    //! The raw thermogram. Claude Generated (WP5)
    inline const XYTrace& Trace() const { return m_trace; }
    inline void setPeakList(const std::vector<PeakPick::Peak>& peak_list) { m_peak_list = QVector<PeakPick::Peak>(peak_list.begin(), peak_list.end()); }
    inline void setPeakList(const QVector<PeakPick::Peak>& peak_list) { m_peak_list = peak_list; }

    void UpdateParameter(const QJsonObject& parameter);

    void Initialise();

    //! The libpeakpick view of Trace() the algorithms run on (index/time conversion for the GUI).
    inline const PeakPick::spectrum* Spectrum() const { return &m_spectrum; }

    inline const QVector<PeakPick::Peak>* Peaks() const { return &m_peak_list; }

    inline QVector<qreal> Integrals() const { return m_integrals_raw; }
    inline QVector<qreal> IntegralsScaled() const { return m_integrals_scaled; }

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

    void setThermogramParameter(const QJsonObject& thermogram_parameter)
    {
        m_thermogram_parameter = thermogram_parameter;
        LoadBlock();
    }
    QJsonObject getThermogramParameter() const;

    void setCalibrationHeat(qreal heat) { m_CalibrationHeat = heat; }
    void setConstantOffset(qreal offset) { m_constant_offset = offset; }
    //! Set the cal->J (or arbitrary) heat scaling factor applied on top of the calibration ratio. Claude Generated
    void setScalingFactor(qreal factor) { m_scaling_factor = factor; }
    qreal ScalingFactor() const { return m_scaling_factor; }
    qreal ConstantOffset() const { return m_constant_offset; }

    void UpdatePeaks();
    void FitBaseLine();

    qreal Calibration() const { return m_calibration_peak.integ_num; }

    //! The last baseline fit (polynomial coefficients and grid points per peak), read-only. Claude Generated
    inline const PeakPick::BaseLineResult& BaseLine() const { return m_baseline; }

    //! Chart points of the raw thermogram. Claude Generated (WP5: computed, no longer stored)
    inline QList<QPointF> ThermogramSeries() const { return m_trace.points(); }
    //! Baseline inside the integration ranges, thinned to ~50 points per peak. Claude Generated (WP5)
    QList<QPointF> BaselineSeries() const;
    //! Grid points of the baseline fit. Claude Generated (WP5)
    QList<QPointF> BaselineGrid() const;
    //! Baseline between the end of an integration range and the end of its peak. Claude Generated (WP5)
    QList<QPointF> BaselineIgnored() const;

    /*! \brief Whether getThermogramParameter() writes the raw curve ("Thermogram").
     *
     * Unset (the default), the application property `StoreRawData` decides, as before; set, this
     * handler follows the setter. The only place the handler reads that property. Claude Generated (WP5) */
    inline void setStoreRawData(bool store) { m_store_raw_data = store; }
    bool StoreRawData() const;

    void ConvertRules();
    void CompressRules();

    inline void setCurrentCutOption(const QString& current_integration_scheme) { m_current_integration_scheme = current_integration_scheme; }
    inline void setInitialThreshold(double initial_threshold) { m_initial_threshold = initial_threshold; }
    inline void setMaximalIterations(int iterations) { m_iterations = iterations; }
    inline void setOvershotCounter(int overshot_counter) { m_overshot_counter = overshot_counter; }
    inline void setCutBefore(bool cut_before) { m_cut_before = cut_before; }
    inline int LastIterations() const { return m_last_iteration_max; }

    inline void ResetThreshold() { m_integration_range_threshold = m_initial_threshold; }

    void ApplyScaling();

private:
    QVector<QPointF> m_peak_rules;
    QVector<qreal> m_integrals_raw, m_integrals_scaled;
    QVector<PeakPick::Peak> m_peak_list;
    XYTrace m_trace; //!< the raw thermogram (primary state)
    PeakPick::spectrum m_spectrum; //!< built from m_trace in setThermogram(), input of libpeakpick
    PeakPick::Peak m_calibration_peak;
    PeakPick::BaseLineResult m_baseline;

    QJsonObject m_thermogram_parameter;

    int m_last_iteration_max = 0, m_iterations = 0, m_overshot_counter = 1, m_PeakCount = 0;
    qreal m_ThermogramBegin = 0, m_ThermogramEnd = 0, m_PeakDuration = 0, m_CalibrationStart = 0, m_constant_offset = 0, m_CalibrationHeat = 0, m_calibration_ratio = 0, m_scaling_factor = 1;
    qreal m_offset = 0, m_frequency = 1, m_initial_threshold = -1, m_integration_range_threshold = 0;
    bool m_initialised = false, m_cut_before = false, m_rules_imported = false, m_averaged = true;
    std::optional<bool> m_store_raw_data; //!< unset: follow the application property StoreRawData
    QString m_current_integration_scheme = QString("Custom"), m_last_integration_scheme;
    QStringList m_integration_scheme = QStringList() << "Custom"
                                                     << "Zero"
                                                     << "Threshold";

    void LegacyLoad();
    /*! \brief Raw curve of a stored block: lossless (TraceJson) and the old 6-digit strings. Claude Generated (WP5) */
    void LoadStoredThermogram(const QJsonObject& thermogram);
    /*! \brief The three baseline chart series in one pass (the former UpdateBaseLine body). Claude Generated (WP5) */
    void BaselinePoints(QList<QPointF>* series, QList<QPointF>* grid, QList<QPointF>* ignored) const;
    void LoadParameter();
    void LoadBlock();
    double ResizeIntegrationRange(double threshold, int direction);
    void ApplyThermogramIntegration();
    void UpdateBaseLine();

signals:
    void ThermogramInitialised();
    void ThermogramChanged();
    void BaseLineChanged();
    void CalibrationChanged();
    void PeakRulesChanged();
    void Message(const QString& message);
};
