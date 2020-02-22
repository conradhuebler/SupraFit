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

ThermogramHandler::ThermogramHandler()
{
}

void ThermogramHandler::Initialise()
{

    if (!m_thermogram_parameter.isEmpty()) {
        if (m_thermogram_parameter.contains("thermogram")) {
            Vector x, y;
            x = ToolSet::String2DoubleEigVec(m_thermogram_parameter["thermogram"].toObject()["x"].toString());
            y = ToolSet::String2DoubleEigVec(m_thermogram_parameter["thermogram"].toObject()["y"].toString());
            m_spectrum.setSpectrum(x, y);

            m_frequency = m_spectrum.Step();

            m_calibration_start = (m_thermogram_parameter["calibration_start"].toDouble());
            m_calibration_heat = (m_thermogram_parameter["calibration_heat"].toDouble());

            m_thermogram_begin = (m_spectrum.XMax());
            m_thermogram_end = (m_spectrum.XMax() - m_calibration_start * m_spectrum.Step());

            emit Message(QString("<font color='red'>The raw data files are not in place. I will use the stored thermogram.</font>"));
        }

        m_constant_offset = (m_thermogram_parameter["constants"].toDouble());
        m_integration_range_threshold = (m_thermogram_parameter["integration_range_threshold"].toDouble());
        // m_integration_range->setCurrentText(m_thermogram_parameter["integration_range"].toString());
        m_initial_threshold = m_thermogram_parameter["integration_range"].toDouble();

        m_thermogram_begin = m_thermogram_parameter["start_time"].toDouble();
        m_peak_time = m_thermogram_parameter["peak_time"].toDouble();
        m_thermogram_end = m_thermogram_parameter["end_time"].toDouble();
        m_iterations = m_thermogram_parameter["iter"].toInt();

        m_peak_rules = ToolSet::String2PointsVector(m_thermogram_parameter["rules_list"].toString());
        UpdatePeaks();
        QVector<QPointF> points = ToolSet::String2PointsVector(m_thermogram_parameter["peak_int_ranges"].toString());
        for (std::size_t i = 0; i < m_peak_list.size(); ++i) {
            m_peak_list[i].int_start = points[i].x();
            m_peak_list[i].int_end = points[i].y();
        }
    }
    if (m_spectrum.size() == 0) {
        std::cout << "No Thermogram found. ThermogramHandler is not initialised!" << std::endl;
        return;
    }

    if (m_peak_list.size() == 0) {
        std::cout << "No Peak list found. Automatic Peak Picking will most probably be not successfull, therefore: ThermogramHandler is not initialised!" << std::endl;
        return;
    } else {
        m_frequency = m_spectrum.Step();
        m_peak_rules.clear();
        double time_pred = 0;
        double peak_start = 0, peak_end = 0, peak_time = 0;
        for (int i = 0; i < m_peak_list.size(); ++i) {
            double time = m_peak_list[i].end * m_spectrum.Step() - m_peak_list[i].start * m_spectrum.Step() + m_spectrum.Step();
            double start = m_peak_list[i].start * m_spectrum.Step() - m_spectrum.Step();
            if (!qFuzzyCompare(time_pred, time)) {
                if (peak_start == 0)
                    m_thermogram_begin = start;
                peak_start = start;
                peak_time = time;
                peak_end = m_peak_list[m_peak_list.size() - 1].end * m_spectrum.Step();
                m_peak_rules.append(QPointF(start, time));
            }
            time_pred = time;
        }
        m_thermogram_end = peak_end;
        m_peak_time = peak_time;
    }
    FitBaseLine();
    m_initialised = true;
}

void ThermogramHandler::UpdatePeaks()
{
    if (m_spectrum.Step() == 0)
        return;

    qreal offset = 0;
    int off = 1;
    int rules_size = m_peak_rules.size();
    double end = m_thermogram_end;
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

        for (int i = index_start; i + (timestep)-1 < m_spectrum.XtoIndex(index_end); i += (timestep)) {
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

    // ResetGuideLabel();
}

void ThermogramHandler::IntegrateThermogram()
{

    if (!m_initialised) {
        std::cout << "ThermogramHandler is not initialised, sorry for that." << std::endl;
        return;
    }
    CalibrateSystem();

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

    int counter = 0;
    double sum = 0;

    for (int i = 0; i < difference_signal_baseline.size(); ++i) {
        tmp << difference_signal_baseline[i] * (difference_signal_baseline[i] < stdev);
        counter += (difference_signal_baseline[i] < stdev);
        sum += difference_signal_baseline[i] * (difference_signal_baseline[i] < stdev);
    }

    qreal stdev2 = Stddev(tmp, 0, sum / double(counter));

    if (m_initial_threshold < 1e-12)
        m_initial_threshold = stdev2 / 10.0;

    m_integration_range_threshold = stdev2 / 10.0;
}

void ThermogramHandler::FitBaseLine()
{
    std::vector<PeakPick::Peak> peak_list = m_peak_list.toStdVector();
    PeakPick::BaseLine baseline(&m_spectrum);
    baseline.setBaseLineRange(PeakPick::BaseLine::BLR::PeakWise);
    baseline.setPolynomFit(PeakPick::BaseLine::Polynom::Slow);
    baseline.setNoCoeffs(2);
    baseline.setPeaks(&peak_list);
    m_baseline = baseline.Fit();

    /*
    if (m_baseline.baselines.size() == 1) {
        m_initial_baseline = m_baseline.baselines[0];
        m_coeffs->setValue(m_initial_baseline.size());
    }*/
}

void ThermogramHandler::CalibrateSystem()
{
    if (m_spectrum.Step() == 0)
        return;

    m_calibration_peak.setPeakStart(m_spectrum.size() - 1 - m_calibration_start * m_spectrum.Step());
    m_calibration_peak.setPeakEnd(m_spectrum.size() - 1);

    std::vector<PeakPick::Peak> list;
    list.push_back(m_calibration_peak);

    PeakPick::spectrum* spectrum = new PeakPick::spectrum(m_spectrum);

    PeakPick::BaseLine baseline(spectrum);

    baseline.setBaseLineRange(PeakPick::BaseLine::BLR::PeakWise);
    baseline.setNoCoeffs(2);
    baseline.setPeaks(&list);
    baseline.setInitialBaseLine(Vector(0));

    PeakPick::BaseLineResult baseline_result = baseline.Fit();

    m_calibration_peak.integ_num = PeakPick::IntegrateNumerical(spectrum, m_calibration_peak.start, m_calibration_peak.end, baseline_result.baselines[0]);
    delete spectrum;
}

QJsonObject ThermogramHandler::getThermogramParameter() const
{
    QJsonObject fit;

    fit["constants"] = m_constant_offset;
    fit["frequency"] = m_frequency;
    fit["start_time"] = m_thermogram_begin;
    fit["end_time"] = m_thermogram_end;
    fit["peak_time"] = m_peak_time;
    fit["calibration_start"] = m_calibration_start;
    fit["calibration_heat"] = m_calibration_heat;

    fit["rules_list"] = ToolSet::Points2String(m_peak_rules);

    QVector<QPointF> points;
    for (int i = 0; i < m_peak_list.size(); ++i)
        points << QPointF(m_peak_list[i].int_start, m_peak_list[i].int_end);
    fit["peak_int_ranges"] = ToolSet::Points2String(points);

    // fit["integration_range"] = m_integration_range->currentText();
    fit["integration_range_threshold"] = m_integration_range_threshold;
    fit["iter"] = m_last_iteration_max;

    if (qApp->instance()->property("StoreRawData").toBool()) {
        QJsonObject thermo;
        thermo["x"] = ToolSet::DoubleList2String(m_spectrum.x());
        thermo["y"] = ToolSet::DoubleList2String(m_spectrum.y());
        fit["thermogram"] = thermo;
    }
    return fit;
}

void ThermogramHandler::AdjustIntegrationRange()
{

    //Integrate(&m_peak_list, &m_spectrum);
    IntegrateThermogram();
    double threshold = 0;
    double old_threshold = m_integration_range_threshold;
    int maxiter = 1;
    int direction = -1 * m_cut_before;
    /* Zero/Threshold Cutting */
    if (m_current_cut_option == m_cut_options[0]) {
        for (int i = 0; i < m_peak_list.size(); ++i) {
            m_peak_list[i].int_start = m_peak_list[i].start;
            m_peak_list[i].int_end = m_peak_list[i].end;
        }
        FitBaseLine();
        m_integration_range_threshold = m_initial_threshold;
        //Integrate(&m_peak_list, &m_spectrum);
        IntegrateThermogram();

        //Update();
        return;
    } else if (m_current_cut_option == m_cut_options[2]) { /* Threshold cutting */
        threshold = m_integration_range_threshold;
        maxiter = m_iterations;
    } else { /* Zero Cutting */
        for (int i = 0; i < m_peak_list.size(); ++i) {
            m_peak_list[i].int_start = m_peak_list[i].start;
            m_peak_list[i].int_end = m_peak_list[i].end;
        }
        FitBaseLine();
        //Integrate(&m_peak_list, &m_spec);
        IntegrateThermogram();

        m_integration_range_threshold = m_initial_threshold;
    }
    Vector baseline;
    if (m_baseline.baselines.size() > 0)
        baseline = m_baseline.baselines[0];
    int x = 0;

    for (x = 0; x < maxiter; ++x) {
        for (int i = 0; i < m_peak_list.size(); ++i) {
            if (m_peak_list.size() == m_baseline.baselines.size()) // && m_baseline.x_grid_points.size() > 0)
                baseline = m_baseline.baselines[i];

            qreal start_end = (m_spectrum.Y(m_peak_list[i].start) + m_spectrum.Y(m_peak_list[i].end)) * 0.5;

            int start_position = m_peak_list[i].max;
            if (qAbs(qAbs(m_spectrum.Y(m_peak_list[i].min) - start_end)) > qAbs(qAbs(m_spectrum.Y(m_peak_list[i].max)) - start_end))
                start_position = m_peak_list[i].min;

            PeakPick::ResizeIntegrationRange(&m_spectrum, &m_peak_list[i], baseline, start_position, threshold, m_overshot_counter, direction);
        }
        FitBaseLine();
        //Integrate(&m_peak_list, &m_spec);
        IntegrateThermogram();

        if (qAbs(old_threshold - m_integration_range_threshold) < 1e-10)
            break;
        old_threshold = m_integration_range_threshold;
    }
    m_last_iteration_max = x;
    FitBaseLine();
    // Update();
    // setGuideText(QString("Adaption of the baseline took %1 cycles").arg(m_last_iteration_max));
}
