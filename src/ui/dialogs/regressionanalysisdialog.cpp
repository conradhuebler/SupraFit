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
 */

#include <charts.h>

#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QScatterSeries>

#include <QtCore/QDebug>
#include <QtCore/QMap>
#include <QtCore/QMultiHash>

#include <QtWidgets/QComboBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLayout>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QTextEdit>

#include <libpeakpick/baseline.h>
#include <libpeakpick/nxlinregress.h>
#include <libpeakpick/peakpick.h>

#include "src/core/libmath.h"

#include "src/ui/guitools/chartwrapper.h"
#include "src/ui/guitools/instance.h"

#include "regressionanalysisdialog.h"

RegressionAnalysisDialog::RegressionAnalysisDialog(QWeakPointer<DataClass> data, QWeakPointer<ChartWrapper> wrapper, QWidget* parent)
    : QDialog(parent)
    , m_data(data)
    , m_wrapper(wrapper)
{
    setUI();
    resize(800, 600);
}

RegressionAnalysisDialog::~RegressionAnalysisDialog()
{
}

void RegressionAnalysisDialog::setUI()
{
    m_fit = new QPushButton(tr("Fit linear"));
    m_functions = new QSpinBox;
    m_functions->setRange(1, 1);

    m_chart = new ListChart;
    connect(m_chart, &ListChart::LastDirChanged, this, [](const QString& str) {
        setLastDir(str);
    });
    connect(Instance::GlobalInstance(), &Instance::ConfigurationChanged, m_chart, &ListChart::ApplyConfigurationChange);

    m_output = new QTextEdit;
    m_lists = new QListWidget;
    m_lists->setMaximumWidth(230);
    connect(m_lists, &QListWidget::currentRowChanged, this, &RegressionAnalysisDialog::LoadRegression);

    QSplitter* splitter = new QSplitter(Qt::Vertical);
    splitter->addWidget(m_chart);

    m_method = new QComboBox;
    m_method->addItem("Trial and Error");
    m_method->addItem("Fast Guess");

    QWidget* results = new QWidget;
    QHBoxLayout* hlayout = new QHBoxLayout;
    hlayout->addWidget(m_output);
    hlayout->addWidget(m_lists);
    results->setLayout(hlayout);
    splitter->addWidget(results);

    QGridLayout* layout = new QGridLayout;
    layout->addWidget(new QLabel(tr("Number of functions")), 0, 0);
    layout->addWidget(m_functions, 0, 1);
    layout->addWidget(m_method, 0, 2);
    layout->addWidget(m_fit, 0, 3);
    layout->addWidget(splitter, 1, 0, 1, 4);
    setLayout(layout);

    connect(m_fit, &QPushButton::clicked, this, &RegressionAnalysisDialog::FitFunctions);
    connect(m_wrapper.data(), &ChartWrapper::ModelTransformed, this, &RegressionAnalysisDialog::UpdatePlots);
}

void RegressionAnalysisDialog::UpdatePlots()
{
    m_chart->Clear();
    m_output->clear();
    m_series = m_wrapper.data()->CloneSeries();
    for (int i = 0; i < m_series.size(); ++i) {
        m_chart->addSeries(m_series[i], i, m_series[i]->color(), Print::printDouble(i + 1));
        m_series[i]->setBorderColor(m_series[i]->color());
        m_series[i]->setMarkerSize(4);
    }
    m_functions->setRange(1, m_series.first()->points().size() / 2);
    m_chart->setXAxis(m_wrapper.data()->XLabel());
    m_chart->setYAxis(m_wrapper.data()->YLabel());

    // TestPeaks();
}

void RegressionAnalysisDialog::TestPeaks()
{
    std::vector<double> x, y;
    for (int i = 0; i < m_series.size(); ++i) {
        QList<QPointF> points = m_series[i]->points();
        for (int j = 0; j < points.size(); ++j) {
            x.push_back(points[j].x());
            y.push_back(points[j].y());
        }
        Vector y_vec = Vector::Map(&y[0], x.size());
        PeakPick::spectrum spectrum(y_vec, x[0], x[x.size() - 1]);
        PeakPick::BaseLine baseline(&spectrum);
        baseline.setNoCoeffs(2);
        baseline.Fit(PeakPick::BaseLine::StartEnd);
        PeakPick::spectrum corrected = baseline.Corrected();
        qDebug() << corrected.PosMax() << corrected.PosMin();

        PeakPick::SmoothFunction(&corrected, 12);
        PeakPick::SmoothFunction(&corrected, 12);
        PeakPick::SmoothFunction(&corrected, 12);
        ScatterSeries* series = new ScatterSeries;
        series->setMarkerSize(1);
        series->setBorderColor(m_series[i]->color());
        for (unsigned int i = 0; i < corrected.size(); ++i)
            series->append(corrected.X(i), corrected.Y(i));
        m_chart->addSeries(series, i, m_series[i]->color());
        std::vector<PeakPick::Peak> peaks = PeakPick::PickPeaks(&corrected, 0, 0.25);
        corrected.InvertSgn();
        std::vector<PeakPick::Peak> peaks2 = PeakPick::PickPeaks(&corrected, 0, 1);
        for (unsigned int i = 0; i < peaks2.size(); ++i)
            peaks.push_back(peaks2[i]);
        m_peaks = peaks;
        for (unsigned int i = 0; i < peaks.size(); ++i) {
            IntegrateNumerical(&corrected, peaks[i]);
            m_peak_list[qAbs(peaks[i].integ_num)] = i;
        }
        qDebug() << m_peak_list;
    }
}

void RegressionAnalysisDialog::FitFunctions()
{
    m_lists->clear();
    m_result.clear();
    int size = 10;
    QVector<qreal> sums(size, 0);

    for (int i = 0; i < m_series.size(); ++i) {
        QVector<qreal> x, y;
        QList<QPointF> points = m_series[i]->points();
        for (int j = 0; j < points.size(); ++j) {
            x << points[j].x();
            y << points[j].y();
        }
        QMap<qreal, PeakPick::MultiRegression> result;
        if (m_method->currentIndex() == 0)
            result = LeastSquares(x, y, m_functions->value());
        else if (m_method->currentIndex() == 1) {

            if (m_functions->value() == 3) {
                std::vector<double> x, y;
                for (int i = 0; i < m_series.size(); ++i) {
                    QList<QPointF> points = m_series[i]->points();
                    for (int j = 0; j < points.size(); ++j) {
                        x.push_back(points[j].x());
                        y.push_back(points[j].y());
                    }
                    Vector y_vec = Vector::Map(&y[0], x.size());
                    PeakPick::spectrum spectrum(y_vec, x[0], x[x.size() - 1]);
                    PeakPick::BaseLine baseline(&spectrum);
                    baseline.setNoCoeffs(2);
                    baseline.Fit(PeakPick::BaseLine::StartEnd);
                    PeakPick::spectrum corrected = baseline.Corrected();
                    qDebug() << corrected.PosMax() << corrected.PosMin();
                }
            } else
                return;

            /*
            int index = 1;
            QMap<double, int>::const_iterator i = m_peak_list.constBegin();
            while (i != m_peak_list.constEnd() && index <= m_functions->value()) {
                std::cout << i.key() << ": " << i.value() << std::endl;
                std::cout << m_peaks[i.value()].start << " " << m_peaks[i.value()].end << std::endl;
                ++i;
                index++;
            }
            return;*/
        } else
            return;
        if (result.isEmpty())
            continue;
        size = qMin(size, result.size());

        for (int index = 0; index < size; ++index) {
            sums[index] += result.value(result.keys()[index]).sum_err;
        }
        m_result << result;
    }
    for (int i = 0; i < size; ++i)
        m_lists->addItem("Fit Number " + QString::number(i) + ": SSE = " + Print::printDouble(sums[i]));
    LoadRegression(0);
}

void RegressionAnalysisDialog::LoadRegression(int index)
{
    if (index < 0)
        return;
    UpdatePlots();
    QString output;
    QVector<qreal> x;

    QList<QPointF> points = m_series.first()->points();
    for (int j = 0; j < points.size(); ++j) {
        x << points[j].x();
    }

    for (int i = 0; i < m_series.size(); ++i) {
        QString latex = QString::number(i + 1) + "&";
        QString intersec = QString();
        PeakPick::MultiRegression regression = m_result[i].value(m_result[i].keys()[index]);
        output += "<h4>Series " + Print::printDouble(i + 1) + "</h4>";
        //qDebug() << regression.start;
        for (unsigned int m = 0; m < regression.regressions.size(); ++m) {
            QtCharts::QLineSeries* series = m_chart->addLinearSeries(regression.regressions[m].m, regression.regressions[m].n, x[regression.start[2 * m - m]], x[regression.start[2 * m + regression.regressions.size() - m]], i);
            series->setColor(m_series[i]->color());
            m_linear_series.insert(i, series);
            output += "<p>y(" + QString::number(m) + ") = " + Print::printDouble(regression.regressions[m].m) + "x + " + Print::printDouble(regression.regressions[m].n) + " ( R<sup>2</sup>=" + Print::printDouble(regression.regressions[m].R) + ", SSE = " + Print::printDouble(regression.regressions[m].sum_err) + ")</ p>";
            output += QString("<p> x<sub>0</sub> = %1 Points: %2 ... %3</p>").arg(Print::printDouble((0 - regression.regressions[m].n) / (regression.regressions[m].m))).arg(regression.start[2 * m] + 1).arg(regression.start[2 * m + 1] + 1); //" + Print::printDouble((0 - regression.regressions[m].n) / (regression.regressions[m].m)) + "<p>";
            latex += QString("%1x +%2 & %3 & %4 & %5...%6&").arg(Print::printDouble(regression.regressions[m].m, 3)).arg(Print::printDouble(regression.regressions[m].n, 3)).arg(Print::printDouble(regression.regressions[m].R, 3)).arg(Print::printDouble(regression.regressions[m].sum_err)).arg(regression.start[2 * m] + 1).arg(regression.start[2 * m + 1] + 1);
            //output += QString("<p>Points from %1 to %2</p>").arg(regression.start[2*m] + 1).arg(regression.start[2*m+1] + 1);
            if (regression.regressions.size() >= 2 && m < regression.regressions.size() - 1) {
                qreal x = (regression.regressions[m].n - regression.regressions[m + 1].n) / (regression.regressions[m + 1].m - regression.regressions[m].m);
                qreal y = regression.regressions[m + 1].m * x + regression.regressions[m + 1].n;
                output += "<p>Intersection of function " + Print::printDouble(m) + " and function " + Print::printDouble(m + 1) + " at (<font color='red'>" + Print::printDouble(x) + ";" + Print::printDouble(y) + "</font>)</p>";
                intersec += QString("(%1;%2)").arg(Print::printDouble(x, 3)).arg(Print::printDouble(y, 3));
            }
        }
        if (regression.regressions.size() != 1)
            output += "<p>Sum of SSE for all function = " + Print::printDouble(regression.sum_err) + "<p>";
        output += QString("<p>%1 %2 \\\\</p>").arg(latex).arg(intersec);
    }
    m_output->append(output);
}
