/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2018  Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <QDebug>

#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QScatterSeries>

#include <QtCore/QMap>
#include <QtCore/QMultiHash>

#include <QtWidgets/QLabel>
#include <QtWidgets/QLayout>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QTextEdit>

#include "src/core/libmath.h"

#include "src/ui/guitools/chartwrapper.h"
#include "src/ui/widgets/listchart.h"

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

    m_output = new QTextEdit;
    m_lists = new QListWidget;
    m_lists->setMaximumWidth(230);
    connect(m_lists, &QListWidget::currentRowChanged, this, &RegressionAnalysisDialog::LoadRegression);

    QSplitter* splitter = new QSplitter(Qt::Vertical);
    splitter->addWidget(m_chart);

    QWidget* results = new QWidget;
    QHBoxLayout* hlayout = new QHBoxLayout;
    hlayout->addWidget(m_output);
    hlayout->addWidget(m_lists);
    results->setLayout(hlayout);
    splitter->addWidget(results);

    QGridLayout* layout = new QGridLayout;
    layout->addWidget(new QLabel(tr("Number of functions")), 0, 0);
    layout->addWidget(m_functions, 0, 1);
    layout->addWidget(m_fit, 0, 2);
    layout->addWidget(splitter, 1, 0, 1, 3);
    setLayout(layout);

    connect(m_fit, &QPushButton::clicked, this, &RegressionAnalysisDialog::FitFunctions);
    connect(m_wrapper.data(), &ChartWrapper::ModelTransformed, this, &RegressionAnalysisDialog::UpdatePlots);
}

void RegressionAnalysisDialog::UpdatePlots()
{
    m_chart->Clear();
    m_output->clear();
    m_series = m_wrapper.data()->CloneSeries();
    for (int i = 0; i < m_series.size(); ++i)
        m_chart->addSeries(m_series[i], i, m_series[i]->color(), Print::printDouble(i + 1));
    m_functions->setRange(1, m_series.first()->points().size() / 2);
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
        QMap<qreal, MultiRegression> result = LeastSquares(x, y, m_functions->value());
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

        MultiRegression regression = m_result[i].value(m_result[i].keys()[index]);
        output += "<h4>Series " + Print::printDouble(i + 1) + "</h4>";
        for (int m = 0; m < regression.regressions.size(); ++m) {
            QtCharts::QLineSeries* series = m_chart->addLinearSeries(regression.regressions[m].m, regression.regressions[m].n, x[regression.start[2 * m - m]], x[regression.start[2 * m + regression.regressions.size() - m]], i);
            series->setColor(m_series[i]->color());
            m_linear_series.insert(i, series);
            output += "<p>y(" + QString::number(m) + ") = " + Print::printDouble(regression.regressions[m].m) + "x + " + Print::printDouble(regression.regressions[m].n) + " ( R<sup>2</sup>=" + Print::printDouble(regression.regressions[m].R) + ", SSE = " + Print::printDouble(regression.regressions[m].sum_err) + ")</ p>";
            output += "<p> x<sub>0</sub> = " + Print::printDouble((0 - regression.regressions[m].n) / (regression.regressions[m].m)) + "<p>";
            if (regression.regressions.size() >= 2 && m < regression.regressions.size() - 1) {
                qreal x = (regression.regressions[m].n - regression.regressions[m + 1].n) / (regression.regressions[m + 1].m - regression.regressions[m].m);
                qreal y = regression.regressions[m + 1].m * x + regression.regressions[m + 1].n;
                output += "<p>Intersection of function " + Print::printDouble(m) + " and function " + Print::printDouble(m + 1) + " at (<font color='red'>" + Print::printDouble(x) + "," + Print::printDouble(y) + "</font>)</p>";
            }
        }
        if (regression.regressions.size() != 1)
            output += "<p>Sum of SSE for all function = " + Print::printDouble(regression.sum_err) + "<p>";
    }
    m_output->append(output);
}
