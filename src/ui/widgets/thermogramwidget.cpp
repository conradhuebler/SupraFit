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

#include <Eigen/Dense>

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTableWidget>

#include <QtCharts/QChart>

#include "libpeakpick/peakpick.h"

#include "src/ui/guitools/chartwrapper.h"
#include "src/ui/widgets/chartview.h"

#include "src/core/toolset.h"
#include "src/global.h"

#include "thermogramwidget.h"

ThermogramWidget::ThermogramWidget(QWidget* parent)
    : QWidget(parent)
{
    setUi();
}

void ThermogramWidget::setUi()
{
    m_data = new QtCharts::QChart();
    m_thermogram = new ChartView(m_data);
    m_thermogram->setModal(true);
    m_thermogram->setMinimumSize(800, 600);

    m_table = new QTableWidget;
    m_table->setFixedWidth(250);

    QHBoxLayout* hlayout = new QHBoxLayout;

    hlayout->addWidget(m_thermogram);
    hlayout->addWidget(m_table);
    setLayout(hlayout);
}

void ThermogramWidget::UpdateTable()
{
    m_table->clear();
    m_table->setRowCount(m_peak_list.size());
    m_table->setColumnCount(2);
    for (int j = 0; j < m_peak_list.size(); ++j) {
        QTableWidgetItem* newItem;
        newItem = new QTableWidgetItem(QString::number(m_spec->X(m_peak_list[j].start)) + " - " + QString::number(m_spec->X(m_peak_list[j].end)));
        m_table->setItem(j, 0, newItem);
        newItem = new QTableWidgetItem(QString::number(m_peak_list[j].integ_num * m_scale));
        m_table->setItem(j, 1, newItem);
    }
}

void ThermogramWidget::UpdatePlot()
{
    m_thermogram_series = fromSpectrum(m_spec);
    m_thermogram->addSeries(m_thermogram_series);
}

void ThermogramWidget::PickPeaks()
{
    if (!m_spectrum)
        return;

    PeakPick::spectrum sign = PeakPick::spectrum(m_spec);
    sign.InvertSgn();

    std::vector<PeakPick::Peak> peaks = PeakPick::PickPeaks(&sign, 0, qPow(2, 1));
    for (int i = 0; i < peaks.size(); ++i) {
        int pos = PeakPick::FindMinimum(m_spec, peaks[i]);
        peaks[i].max = pos;
        PeakPick::IntegrateNumerical(m_spec, peaks[i]);
    }

    std::vector<PeakPick::Peak> max_peak = PeakPick::PickPeaks(m_spec, 0, qPow(2, 1));

    for (int i = 0; i < max_peak.size(); ++i) {
        int pos = PeakPick::FindMaximum(m_spec, max_peak[i]);
        max_peak[i].max = pos;
        PeakPick::IntegrateNumerical(m_spec, max_peak[i]);
    }
    peaks.insert(peaks.end(), max_peak.begin(), max_peak.end());

    m_table->clear();
    m_table->setRowCount(peaks.size());
    m_table->setColumnCount(2);
    for (int j = 0; j < peaks.size(); ++j) {
        int pos = peaks[j].max;
        QTableWidgetItem* newItem;
        newItem = new QTableWidgetItem(QString::number(m_spec->X(pos)));
        m_table->setItem(j, 0, newItem);
        newItem = new QTableWidgetItem(QString::number(peaks[j].integ_num));
        m_table->setItem(j, 1, newItem);
    }
    m_peak_list = peaks;
    UpdateTable();
}

LineSeries* ThermogramWidget::fromSpectrum(const PeakPick::spectrum* original)
{
    LineSeries* series = new LineSeries;
    for (int i = 0; i < original->size(); i++)
        series->append(QPointF(original->X(i), original->Y(i) * m_scale));

    return series;
}

void ThermogramWidget::clear()
{
    m_peak_list.clear();
    m_spectrum = false;
    delete m_spec;
    m_table->clear();
    m_thermogram->ClearChart();
}

void ThermogramWidget::Update()
{
    Integrate(&m_peak_list, m_spec);
    UpdateTable();
}

void ThermogramWidget::Integrate(std::vector<PeakPick::Peak>* peaks, const PeakPick::spectrum* original)
{
    m_peaks.clear();
    for (int i = 0; i < peaks->size(); ++i) {
        int min = PeakPick::FindMinimum(original, (*peaks)[i]);
        int max = PeakPick::FindMaximum(original, (*peaks)[i]);
        (*peaks)[i].max = ((*peaks)[i].end + (*peaks)[i].start) / 2.0;
        PeakPick::IntegrateNumerical(original, (*peaks)[i], m_offset);
        m_peaks << (*peaks)[i].integ_num;
    }
}
