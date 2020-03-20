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

#include <Eigen/Dense>

#include <QtCore/QCoreApplication>
#include <QtCore/QObject>
#include <QtCore/QPointF>
#include <QtCore/QVector>

#include "libpeakpick/analyse.h"
#include "libpeakpick/baseline.h"
#include "libpeakpick/peakpick.h"

#include "src/core/instance.h"
#include "src/core/libmath.h"
#include "src/core/toolset.h"

#include "thermogramhandler.h"

static int baseline_step_size = 50;

ThermogramHandler::ThermogramHandler()
{
}

void ThermogramHandler::Initialise()
{

    if (!m_thermogram_parameter.isEmpty()) {

        if (!m_thermogram_parameter.contains("SupraFit"))
            LegacyLoad();
        else
            LoadParameter();
    }
    if (m_spectrum.size() == 0) {
        std::cout << "No Thermogram found. ThermogramHandler is not initialised!" << std::endl;
        return;
    } else {

        for (unsigned int i = 0; i < m_spectrum.x().size(); i++) {
            m_thermogram_series.append(QPointF(m_spectrum.x()[i], m_spectrum.y()[i]));
        }

        if (qFuzzyCompare(m_ThermogramEnd, 0))
            m_ThermogramEnd = m_spectrum.XMax();
        if (qFuzzyCompare(m_ThermogramBegin, 0))
            m_ThermogramBegin = m_spectrum.XMin();
    }

    if (m_peak_list.size() == 0) {
        m_initialised = true;
        if (m_spectrum.size() && m_ThermogramEnd > 0 && m_PeakCount && m_PeakDuration) {
            double end = m_spectrum.XMax();
            m_ThermogramEnd = end - m_CalibrationStart;
            m_ThermogramBegin = m_ThermogramEnd - m_PeakCount * m_PeakDuration;
            m_peak_rules.clear();
            m_peak_rules << QPointF(m_ThermogramBegin, m_PeakDuration);
            UpdatePeaks();
        } else if (m_spectrum.size() && m_peak_list.size() && m_peak_rules.size()) {
            UpdatePeaks();
        } else {
            std::cout << "No Peak list found. Automatic Peak Picking will most probably be not successfull, therefore: ThermogramHandler is initalised but not ready. No automatic stuff here!" << std::endl;
            emit ThermogramChanged();
            return;
        }
    } else {
        CompressRules();
    }
    FitBaseLine();
    m_initialised = true;
    std::cout << "ThermogramHandler initialised!" << std::endl;
    emit ThermogramChanged();
}

void ThermogramHandler::LoadParameter()
{
    if (!m_thermogram_parameter.isEmpty()) {
        if (m_thermogram_parameter.contains("Thermogram")) {
            Vector x, y;
            x = ToolSet::String2DoubleEigVec(m_thermogram_parameter["Thermogram"].toObject()["x"].toString());
            y = ToolSet::String2DoubleEigVec(m_thermogram_parameter["Thermogram"].toObject()["y"].toString());
            m_spectrum.setSpectrum(x, y);

            m_frequency = m_spectrum.Step();

            m_ThermogramBegin = m_spectrum.XMax();
            //m_ThermogramEnd = (m_spectrum.XMax() - m_CalibrationStart * m_spectrum.Step());

            emit Message(QString("<font color='red'>The raw data files are not in place. I will use the stored thermogram.</font>"));
        }

        if (m_thermogram_parameter.contains("PeakCount"))
            m_PeakCount = (m_thermogram_parameter["PeakCount"].toDouble());

        if (m_thermogram_parameter.contains("CalibrationStart"))
            m_CalibrationStart = (m_thermogram_parameter["CalibrationStart"].toDouble());

        if (m_thermogram_parameter.contains("CalibrationHeat"))
            m_CalibrationHeat = (m_thermogram_parameter["CalibrationHeat"].toDouble());

        if (m_thermogram_parameter.contains("ConstantOffset"))
            m_constant_offset = (m_thermogram_parameter["ConstantOffset"].toDouble());

        if (m_thermogram_parameter.contains("ScalingFactor"))
            m_scaling_factor = (m_thermogram_parameter["ScalingFactor"].toDouble());

        if (m_thermogram_parameter.contains("IntegrationRangeThreshold"))
            m_integration_range_threshold = (m_thermogram_parameter["IntegrationRangeThreshold"].toDouble());

        // m_integration_range->setCurrentText(m_thermogram_parameter["integration_range"].toString());

        if (m_thermogram_parameter.contains("IntegrationScheme"))
            m_current_integration_scheme = m_thermogram_parameter["IntegrationScheme"].toString();

        if (m_thermogram_parameter.contains("ThermogramBegin"))
            m_ThermogramBegin = m_thermogram_parameter["ThermogramBegin"].toDouble();

        if (m_thermogram_parameter.contains("ThermogramEnd"))
            m_ThermogramEnd = m_thermogram_parameter["ThermogramEnd"].toDouble();

        if (m_thermogram_parameter.contains("PeakDuration"))
            m_PeakDuration = m_thermogram_parameter["PeakDuration"].toDouble();

        if (m_thermogram_parameter.contains("MaxIteration"))
            m_iterations = m_thermogram_parameter["MaxIteration"].toInt();

        if (m_thermogram_parameter.contains("PeakRuleList"))
            m_peak_rules = ToolSet::String2PointsVector(m_thermogram_parameter["PeakRuleList"].toString());

        if (m_thermogram_parameter.contains("AverageDirection"))
            m_averaged = m_thermogram_parameter["AverageDirection"].toBool();

        if(m_peak_list.size() == 0)
            UpdatePeaks();

         if (m_thermogram_parameter.contains("PeakIntegrationRange"))
         {
            QVector<QPointF> points = ToolSet::String2PointsVector(m_thermogram_parameter["PeakIntegrationRange"].toString());
            if (m_peak_list.size() != points.size())
                return;
            for (std::size_t i = 0; i < m_peak_list.size(); ++i) {
                m_peak_list[i].int_start = points[i].x();
                m_peak_list[i].int_end = points[i].y();
            }
         }else
             AdjustIntegrationRange();
    }
}

void ThermogramHandler::LegacyLoad()
{
    if (m_thermogram_parameter.contains("thermogram")) {
        Vector x, y;
        x = ToolSet::String2DoubleEigVec(m_thermogram_parameter["thermogram"].toObject()["x"].toString());
        y = ToolSet::String2DoubleEigVec(m_thermogram_parameter["thermogram"].toObject()["y"].toString());
        m_spectrum.setSpectrum(x, y);

        m_frequency = m_spectrum.Step();

        //      m_ThermogramBegin = (m_spectrum.XMax());
        //      m_ThermogramEnd = (m_spectrum.XMax() - m_CalibrationStart * m_spectrum.Step());

        emit Message(QString("<font color='red'>The raw data files are not in place. I will use the stored thermogram.</font>"));
    }

    if (m_thermogram_parameter.contains("calibration_start"))
        m_CalibrationStart = (m_thermogram_parameter["calibration_start"].toDouble());

    if (m_thermogram_parameter.contains("calibration_heat"))
        m_CalibrationHeat = (m_thermogram_parameter["calibration_heat"].toDouble());

    if (m_thermogram_parameter.contains("constants"))
        m_constant_offset = (m_thermogram_parameter["constants"].toDouble());

    if (m_thermogram_parameter.contains("integration_range_threshold"))
        m_integration_range_threshold = (m_thermogram_parameter["integration_range_threshold"].toDouble());

    // m_integration_range->setCurrentText(m_thermogram_parameter["integration_range"].toString());

    if (m_thermogram_parameter.contains("integration_range"))
        m_current_integration_scheme = m_thermogram_parameter["integration_range"].toString();

    if (m_thermogram_parameter.contains("start_time"))
        m_ThermogramBegin = m_thermogram_parameter["start_time"].toDouble();

    if (m_thermogram_parameter.contains("peak_time"))
        m_PeakDuration = m_thermogram_parameter["peak_time"].toDouble();

    if (m_thermogram_parameter.contains("peak_time"))
        m_ThermogramEnd = m_thermogram_parameter["end_time"].toDouble();

    if (m_thermogram_parameter.contains("iter"))
        m_iterations = m_thermogram_parameter["iter"].toInt();

    if (m_thermogram_parameter.contains("rules_list"))
        m_peak_rules = ToolSet::String2PointsVector(m_thermogram_parameter["rules_list"].toString());

    UpdatePeaks();

    if (m_thermogram_parameter.contains("peak_int_ranges")) {
        QVector<QPointF> points = ToolSet::String2PointsVector(m_thermogram_parameter["peak_int_ranges"].toString());
        for (int i = 0; i < m_peak_list.size(); ++i) {
            m_peak_list[i].int_start = points[i].x();
            m_peak_list[i].int_end = points[i].y();
        }
    }
}

void ThermogramHandler::UpdatePeaks()
{
    if (m_spectrum.Step() == 0)
        return;

    qreal offset = 0;
    int off = 1;
    int rules_size = m_peak_rules.size();
    double end = m_ThermogramEnd;
    m_peak_list.clear();
    PeakPick::Peak peak;

    for (int j = 0; j < rules_size; ++j) {

        double index_start = m_spectrum.XtoIndex(m_peak_rules[j].x());
        double timestep = m_peak_rules[j].y() / m_spectrum.Step();

        if (timestep <= 0) {
            emit Message(QString("<font color='red'>Sorry, but Peak rule %1 contains a zero as peak time. That means, if I did not stop right here, you would be waiting for Godot</font>").arg(j + 1));
            return;
        }
        double index_end;

        if (j == rules_size - 1)
            index_end = end;
        else
            index_end = m_peak_rules[j + 1].x();
        int i = 0;
        for (i = index_start; i + timestep < m_spectrum.XtoIndex(index_end) + m_ThermogramBegin; i += (timestep)) {
            peak = PeakPick::Peak();
            peak.setPeakStart(i);
            peak.setPeakEnd(i + (timestep)-1);
            peak.max = PeakPick::FindMaximum(&m_spectrum, peak);
            peak.min = PeakPick::FindMinimum(&m_spectrum, peak);

            m_peak_list.push_back(peak);
        }
    }

    m_offset = offset / double(off);
    m_baseline.baselines.push_back(Vector(1));
    m_baseline.baselines[0](0) = m_offset;
    CalibrateSystem();
    // ResetGuideLabel();
}

void ThermogramHandler::IntegrateThermogram()
{

    if (!m_initialised) {
        std::cout << "ThermogramHandler is not initialised, sorry for that." << std::endl;
        return;
    }
    CalibrateSystem();

    if (m_last_integration_scheme != m_current_integration_scheme)
        AdjustIntegrationRange();

    if (m_averaged == true && m_current_integration_scheme == m_integration_scheme[1]) {

        m_current_integration_scheme = m_integration_scheme[0];
        AdjustIntegrationRange();
        FitBaseLine();

        m_current_integration_scheme = m_integration_scheme[1];
        AdjustIntegrationRange();
        QVector<PeakPick::Peak> cached = m_peak_list;
        ResizeIntegrationRange(0, 0);
        m_peak_list = cached;

        ApplyThermogramIntegration();

        QVector<qreal> integrals = m_integrals_raw;
        for (int i = 0; i < m_peak_list.size(); ++i)
            m_peak_list[i].int_end--;

        ApplyThermogramIntegration();
        m_peak_list = cached;

        for (int i = 0; i < m_integrals_raw.size(); ++i) {
            m_integrals_raw[i] += integrals[i];
            m_integrals_raw[i] /= double(2);
            m_peak_list[i].integ_num = m_integrals_raw[i];
        }
    } else {
        FitBaseLine();
        ApplyThermogramIntegration();
    }
    UpdateBaseLine();
    /*
    for (int i = 0; i < m_integrals_raw.size(); ++i) {
        m_peak_list[i].integ_num *= m_calibration_ratio;
    }*/
    emit CalibrationChanged();
    emit ThermogramChanged();
}

void ThermogramHandler::ApplyThermogramIntegration()
{
    m_integrals_raw.clear();

    Vector baseline;
    QVector<qreal> difference_signal_baseline, tmp;
    qreal sum_difference_signal_baseline = 0;
    if (m_baseline.baselines.size() > 0)
        baseline = m_baseline.baselines[0];

    for (std::size_t i = 0; i < m_peak_list.size(); ++i) {
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        if (m_peak_list.size() == m_baseline.baselines.size()) // && m_baseline.x_grid_points.size() > 0)
        {
            baseline = m_baseline.baselines[i];
        }
        PeakPick::IntegrateNumerical(&m_spectrum, m_peak_list[i], baseline);
        m_integrals_raw << m_peak_list[i].integ_num;

        for (std::size_t j = m_peak_list[i].int_start; j < m_peak_list[i].int_end - 1; j++) {
            sum_difference_signal_baseline += qAbs(PeakPick::Polynomial(m_spectrum.X(j), baseline) - (m_spectrum.Y(j)));
            difference_signal_baseline << qAbs(PeakPick::Polynomial(m_spectrum.X(j), baseline) - (m_spectrum.Y(j)));
        }
    }
    qreal stdev = Stddev(difference_signal_baseline, 0, sum_difference_signal_baseline / double(difference_signal_baseline.size()));

    m_integration_range_threshold = stdev / 100.0;
}

void ThermogramHandler::FitBaseLine()
{
    std::vector<PeakPick::Peak> peak_list = m_peak_list.toStdVector();
    PeakPick::BaseLine base(&m_spectrum);
    base.setBaseLineRange(PeakPick::BaseLine::BLR::PeakWise);
    base.setPolynomFit(PeakPick::BaseLine::Polynom::Slow);
    base.setNoCoeffs(2);
    base.setPeaks(&peak_list);
    m_baseline = base.Fit();
}

void ThermogramHandler::UpdateBaseLine()
{
    m_baseline_grid.clear();
    m_baseline_series.clear();
    m_baseline_ignored_series.clear();

    Vector baseline;
    int steps = 1;

    if (m_baseline.baselines.size() > 0)
        baseline = m_baseline.baselines[0];

    for (int i = 0; i < int(m_peak_list.size()); ++i) {
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        if (m_peak_list.size() == m_baseline.baselines.size() && m_baseline.x_grid_points.size() > 0) {
            baseline = m_baseline.baselines[i];
            for (int j = 0; j < int(m_baseline.x_grid_points[i].size()); ++j) {
                m_baseline_grid << QPointF(m_baseline.x_grid_points[i][j], m_baseline.y_grid_points[i][j]);
            }
        }

        steps = (m_peak_list[i].end - m_peak_list[i].start) / baseline_step_size + 1;

        for (int j = m_peak_list[i].int_start; j < int(m_peak_list[i].int_end) - 1; j += steps)
            m_baseline_series << QPointF(QPointF(m_spectrum.X(j), PeakPick::Polynomial(m_spectrum.X(j), baseline)));

        for (int j = m_peak_list[i].int_end; j < int(m_peak_list[i].end) - 1; j += steps)
            m_baseline_ignored_series.append(QPointF(m_spectrum.X(j), PeakPick::Polynomial(m_spectrum.X(j), baseline)));

        m_baseline_series << QPointF(m_spectrum.X(int(m_peak_list[i].int_end) - 1), PeakPick::Polynomial(m_spectrum.X(int(m_peak_list[i].int_end) - 1), baseline));
        m_baseline_ignored_series.append(QPointF(m_spectrum.X(int(m_peak_list[i].end) - 1), PeakPick::Polynomial(m_spectrum.X(int(m_peak_list[i].end) - 1), baseline)));
    }

    if (m_baseline.x_grid_points.size() == 1) {
        for (int j = 0; j < int(m_baseline.x_grid_points[0].size()); j += steps) {
            m_baseline_grid << QPointF(m_baseline.x_grid_points[0][j], m_baseline.y_grid_points[0][j]);
        }
    }

    emit BaseLineChanged();
}

void ThermogramHandler::CalibrateSystem()
{
    if (m_spectrum.Step() == 0 || qFuzzyCompare(m_CalibrationHeat, 0))
        return;

    m_calibration_peak.setPeakStart(m_spectrum.size() - 1 - m_CalibrationStart);
    m_calibration_peak.setPeakEnd(m_spectrum.size() - 1);

    std::vector<PeakPick::Peak> list;
    list.push_back(m_calibration_peak);

    PeakPick::BaseLine baseline(&m_spectrum);

    baseline.setBaseLineRange(PeakPick::BaseLine::BLR::PeakWise);
    baseline.setNoCoeffs(2);
    baseline.setPeaks(&list);
    baseline.setInitialBaseLine(Vector(0));

    PeakPick::BaseLineResult baseline_result = baseline.Fit();

    m_calibration_peak.integ_num = PeakPick::IntegrateNumerical(&m_spectrum, m_calibration_peak.start, m_calibration_peak.end, baseline_result.baselines[0]);
    m_calibration_ratio = m_CalibrationHeat / m_calibration_peak.integ_num;
    emit CalibrationChanged();
}

QJsonObject ThermogramHandler::getThermogramParameter() const
{
    QJsonObject fit;

    fit["SupraFit"] = qint_version;
    fit["ConstantOffset"] = m_constant_offset;
    fit["ThermogramBegin"] = m_ThermogramBegin;
    fit["ThermogramEnd"] = m_ThermogramEnd;
    fit["PeakDuration"] = m_PeakDuration;
    fit["CalibrationStart"] = m_CalibrationStart;
    fit["CalibrationHeat"] = m_CalibrationHeat;
    fit["ScalingFactor"] = m_scaling_factor;
    fit["AverageDirection"] = m_averaged;
    fit["IntegrationScheme"] = m_current_integration_scheme;
    fit["PeakCount"] = m_peak_list.size();
    fit["CalibrationHeat"] = m_CalibrationHeat;

    fit["PeakRuleList"] = ToolSet::Points2String(m_peak_rules);

    QVector<QPointF> points;
    for (int i = 0; i < m_peak_list.size(); ++i)
        points << QPointF(m_peak_list[i].int_start, m_peak_list[i].int_end);
    fit["PeakIntegrationRange"] = ToolSet::Points2String(points);

    // fit["integration_range"] = m_integration_range->currentText();
    fit["IntegrationRangeThreshold"] = m_integration_range_threshold;
    fit["Iterations"] = m_last_iteration_max;

    if (qApp->instance()->property("StoreRawData").toBool()) {
        QJsonObject thermo;
        thermo["x"] = ToolSet::DoubleList2String(m_spectrum.x());
        thermo["y"] = ToolSet::DoubleList2String(m_spectrum.y());
        fit["Thermogram"] = thermo;
    }
    return fit;
}

void ThermogramHandler::AdjustIntegrationRange()
{
    ApplyThermogramIntegration();
    double threshold = 0;
    int maxiter = 1;
    int direction = -1 * m_cut_before;
    /* Zero/Threshold Cutting */
    if (m_current_integration_scheme == m_integration_scheme[0]) {
        for (int i = 0; i < m_peak_list.size(); ++i) {
            m_peak_list[i].int_start = m_peak_list[i].start;
            m_peak_list[i].int_end = m_peak_list[i].end;
        }
        FitBaseLine();
        m_integration_range_threshold = m_initial_threshold;
        ApplyThermogramIntegration();
        return;
    } else if (m_current_integration_scheme == m_integration_scheme[2]) { /* Threshold cutting */
        threshold = m_integration_range_threshold;
        maxiter = m_iterations;
    } else { /* Zero Cutting */
        for (int i = 0; i < m_peak_list.size(); ++i) {
            m_peak_list[i].int_start = m_peak_list[i].start;
            m_peak_list[i].int_end = m_peak_list[i].end;
        }
        FitBaseLine();
        ApplyThermogramIntegration();
        m_integration_range_threshold = m_initial_threshold;
    }
    Vector baseline;
    if (m_baseline.baselines.size() > 0)
        baseline = m_baseline.baselines[0];

    for (int x = 0; x < maxiter; ++x) {
        double thresh = ResizeIntegrationRange(threshold, direction);
        if (qAbs(thresh - threshold) > 1e-10)
            threshold = thresh;
        else {
            m_last_iteration_max = x;
            break;
        }
    }
    if (m_current_integration_scheme == m_integration_scheme[2]) {
        FitBaseLine();
    }
    m_last_integration_scheme = m_current_integration_scheme;
    emit CalibrationChanged();
}

double ThermogramHandler::ResizeIntegrationRange(double threshold, int direction)
{
    Vector baseline;
    for (int i = 0; i < m_peak_list.size(); ++i) {
        if (m_peak_list.size() == m_baseline.baselines.size()) // && m_baseline.x_grid_points.size() > 0)
            baseline = m_baseline.baselines[i];

        qreal start_end = (m_spectrum.Y(m_peak_list[i].start) + m_spectrum.Y(m_peak_list[i].end)) * 0.5;

        int start_position = m_peak_list[i].max;
        if (qAbs(qAbs(m_spectrum.Y(m_peak_list[i].min) - start_end)) > qAbs(qAbs(m_spectrum.Y(m_peak_list[i].max)) - start_end))
            start_position = m_peak_list[i].min;
        PeakPick::ResizeIntegrationRange(&m_spectrum, &m_peak_list[i], baseline, start_position, threshold, m_overshot_counter, direction);
    }
    if (!qFuzzyIsNull(threshold))
        FitBaseLine();

    ApplyThermogramIntegration();

    return m_integration_range_threshold;
}

void ThermogramHandler::ConvertRules()
{
    m_peak_rules.clear();
    for (int i = 0; i < m_peak_list.size(); ++i) {
        m_peak_rules << QPointF((m_peak_list[i].start) * m_spectrum.Step() + m_spectrum.XMin(), (m_peak_list[i].end - m_peak_list[i].start) * m_spectrum.Step() + m_spectrum.XMin());
    }
    m_rules_imported = true;
    emit PeakRulesChanged();
}

void ThermogramHandler::CompressRules()
{
    m_frequency = m_spectrum.Step();
    m_peak_rules.clear();
    double time_pred = 0;
    double peak_start = 0, peak_end = 0, peak_time = 0;
    for (int i = 0; i < m_peak_list.size(); ++i) {
        double time = m_peak_list[i].end * m_spectrum.Step() - m_peak_list[i].start * m_spectrum.Step() + m_spectrum.Step();
        double start = m_peak_list[i].start * m_spectrum.Step() + m_spectrum.XMin();
        if (!qFuzzyCompare(time_pred, time)) {
            if (peak_start == 0)
                m_ThermogramBegin = start;
            peak_start = start;
            peak_time = time;
            peak_end = m_peak_list[m_peak_list.size() - 1].end * m_spectrum.Step();
            m_peak_rules.append(QPointF(start, time));
        }
        time_pred = time;
    }
    m_ThermogramEnd = peak_end;
    m_PeakDuration = peak_time;
    emit PeakRulesChanged();
}
