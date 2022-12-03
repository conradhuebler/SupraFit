/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2022 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/core/libmath.h"
#include "src/global.h"

#include <QtCore/QTimer>

#include <QtGui/QDoubleValidator>

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLineEdit>

#include "parameterdialog.h"

ParameterDialog::ParameterDialog(const ParameterBoundary& boundary, double value, double SSE, const QString& parameter, QWidget* parent)
    : QDialog(parent)
    , m_boundary(boundary)
    , m_value(value)
    , m_SSE(SSE)
    , m_parameter(parameter)
{
    setWindowTitle(tr("Configure constrained optimisation for %1").arg(m_parameter));
    setUi();
}

void ParameterDialog::setUi()
{
    // QGridLayout* layout = new QGridLayout;

    m_chart = new ChartView;

    m_lowerbound = new LineSeries;
    m_lowerbound->setName("Lower Boundary");
    m_lowerbound->setShowInLegend(true);
    //  m_lowerbound->setLineWidth(8);

    m_upperbound = new LineSeries;
    m_upperbound->setName("Upper Boundary");
    m_upperbound->setShowInLegend(true);
    //   m_upperbound->setLineWidth(8);

    m_combined = new LineSeries;
    m_combined->setName("Joinded Boundaries");
    m_combined->setShowInLegend(true);
    //   m_combined->setLineWidth(4);
    m_combined->setDashDotLine(true);

    m_sse_series = new LineSeries;
    m_sse_series->setName("Current SSE");
    m_sse_series->setShowInLegend(true);

    m_current_value = new LineSeries;
    m_current_value->setName("Current Parameter Value");
    m_current_value->setShowInLegend(true);

    // m_current_penalty = new LineSeries;

    m_chart->addSeries(m_lowerbound);
    m_chart->addSeries(m_upperbound);
    m_chart->addSeries(m_combined);
    m_chart->addSeries(m_sse_series);
    m_chart->addSeries(m_current_value);
    //  m_chart->addSeries(m_current_penalty);

    m_buttonbox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(m_buttonbox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonbox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    m_adopt_and_accept = new QPushButton(tr("Adopt and Accept"));
    connect(m_adopt_and_accept, &QPushButton::clicked, this, &ParameterDialog::Adopt);

    m_value_edit = new QDoubleSpinBox;
    m_value_edit->setDecimals(7);
    m_value_edit->setMinimum(-1e27);
    m_value_edit->setMaximum(1e27);
    m_value_edit->setValue((m_value));
    m_value_edit->setPrefix(QString("%1 = ").arg(m_parameter));
    connect(m_value_edit, &QDoubleSpinBox::valueChanged, this, &ParameterDialog::Recalculate);

    m_general_information = new QLabel;
    m_general_information->setTextFormat(Qt::RichText);
    m_general_information->setTextInteractionFlags(Qt::TextBrowserInteraction);
    m_general_information->setOpenExternalLinks(true);

    m_minimum_edit = new QDoubleSpinBox;
    m_minimum_edit->setDecimals(7);
    m_minimum_edit->setMinimum(-1e27);
    m_minimum_edit->setMaximum(1e27);
    m_minimum_edit->setValue((m_boundary.lower_barrier));
    m_minimum_edit->setPrefix("[  ");
    connect(m_minimum_edit, &QDoubleSpinBox::valueChanged, this, &ParameterDialog::Recalculate);
    QLabel* lowerLabel = new QLabel(tr("Parameters"));
    m_left_boundary = new QLabel;

    m_maximum_edit = new QDoubleSpinBox;
    m_maximum_edit->setDecimals(7);
    m_maximum_edit->setMinimum(-1e27);
    m_maximum_edit->setMaximum(1e27);
    m_maximum_edit->setValue((m_boundary.upper_barrier));
    m_maximum_edit->setSuffix("  ]");
    connect(m_maximum_edit, &QDoubleSpinBox::valueChanged, this, &ParameterDialog::Recalculate);
    QLabel* upperLabel = new QLabel(tr("Parameters"));
    m_right_boundary = new QLabel;

    m_lower_limit = new QCheckBox;
    m_lower_limit->setText(tr("Enable Lower Boundary"));
    m_lower_limit->setChecked(m_boundary.limit_lower);
    connect(m_lower_limit, &QCheckBox::stateChanged, this, [this, lowerLabel](int state) {
        m_minimum_edit->setEnabled(state);
        m_lower_barrier_beta->setEnabled(state);
        m_lower_barrier_wall->setEnabled(state);
        lowerLabel->setEnabled(state);
        m_left_boundary->setEnabled(state);
        Recalculate();
    });

    m_upper_limit = new QCheckBox;
    m_upper_limit->setText(tr("Enable Upper Boundary"));
    m_upper_limit->setChecked(m_boundary.limit_upper);
    connect(m_upper_limit, &QCheckBox::stateChanged, this, [this, upperLabel](int state) {
        m_maximum_edit->setEnabled(state);
        m_upper_barrier_beta->setEnabled(state);
        m_upper_barrier_wall->setEnabled(state);
        upperLabel->setEnabled(state);
        m_right_boundary->setEnabled(state);
        Recalculate();
    });

    m_lower_barrier_beta = new QDoubleSpinBox;
    m_lower_barrier_beta->setRange(0, 1e10);
    m_lower_barrier_beta->setValue((m_boundary.lower_barrier_beta));
    m_lower_barrier_beta->setDecimals(4);
    m_lower_barrier_beta->setPrefix(QString("%1 =").arg(Unicode_beta));
    connect(m_lower_barrier_beta, &QDoubleSpinBox::valueChanged, this, &ParameterDialog::Recalculate);

    m_lower_barrier_wall = new QDoubleSpinBox;
    m_lower_barrier_wall->setRange(0, 1e10);
    m_lower_barrier_wall->setValue((m_boundary.lower_barrier_wall));
    m_lower_barrier_wall->setPrefix(QString("k ="));
    connect(m_lower_barrier_wall, &QDoubleSpinBox::valueChanged, this, &ParameterDialog::Recalculate);

    m_upper_barrier_beta = new QDoubleSpinBox;
    m_upper_barrier_beta->setRange(0, 1e10);
    m_upper_barrier_beta->setValue((m_boundary.upper_barrier_beta));
    m_upper_barrier_beta->setDecimals(4);
    m_upper_barrier_beta->setPrefix(QString("%1 =").arg(Unicode_beta));
    connect(m_upper_barrier_beta, &QDoubleSpinBox::valueChanged, this, &ParameterDialog::Recalculate);

    m_upper_barrier_wall = new QDoubleSpinBox;
    m_upper_barrier_wall->setRange(0, 1e10);
    m_upper_barrier_wall->setValue((m_boundary.upper_barrier_wall));
    m_upper_barrier_wall->setPrefix(QString("k ="));
    connect(m_upper_barrier_wall, &QDoubleSpinBox::valueChanged, this, &ParameterDialog::Recalculate);

    QVBoxLayout* layout = new QVBoxLayout;
    setLayout(layout);

    QHBoxLayout* hlayout = new QHBoxLayout;
    // hlayout->addWidget(new QLabel(tr("Current value of %1").arg(m_parameter)));
    hlayout->addWidget(m_value_edit);
    // hlayout->addStretch();

    layout->addLayout(hlayout);
    layout->addWidget(m_general_information);

    QVBoxLayout* lowerLayout = new QVBoxLayout;
    lowerLayout->addWidget(m_lower_limit);
    lowerLayout->addWidget(m_minimum_edit);
    lowerLayout->addWidget(lowerLabel);
    hlayout = new QHBoxLayout;
    hlayout->addWidget(m_lower_barrier_beta);
    hlayout->addWidget(m_lower_barrier_wall);
    lowerLayout->addLayout(hlayout);
    lowerLayout->addWidget(m_left_boundary);

    QVBoxLayout* upperLayout = new QVBoxLayout;
    upperLayout->addWidget(m_upper_limit);
    upperLayout->addWidget(m_maximum_edit);
    upperLayout->addWidget(upperLabel);
    hlayout = new QHBoxLayout;
    hlayout->addWidget(m_upper_barrier_beta);
    hlayout->addWidget(m_upper_barrier_wall);
    upperLayout->addLayout(hlayout);
    upperLayout->addWidget(m_right_boundary);

    hlayout = new QHBoxLayout;
    hlayout->addLayout(lowerLayout);
    hlayout->addLayout(upperLayout);
    layout->addLayout(hlayout);

    layout->addWidget(m_chart);
    hlayout = new QHBoxLayout;
    hlayout->addWidget(m_adopt_and_accept);
    hlayout->addWidget(m_buttonbox);
    layout->addLayout(hlayout);

    /*
    layout->addWidget(m_value_edit, 0, 2);

    layout->addWidget(m_lower_limit, 1, 0);
    layout->addWidget(m_minimum_edit, 1, 1);
    layout->addWidget(m_upper_limit, 1, 3);
    layout->addWidget(m_maximum_edit, 1, 4);
    layout->addWidget(m_lower_barrier_beta, 2, 0);
    layout->addWidget(m_lower_barrier_wall, 2, 1);
    layout->addWidget(m_upper_barrier_beta, 2, 3);
    layout->addWidget(m_upper_barrier_wall, 2, 4);
    layout->addWidget(m_chart, 3, 0, 1, 5);
    layout->addWidget(m_adopt_and_accept, 4, 0, 1, 3);
    layout->addWidget(m_buttonbox, 4, 3, 1, 2);
    */
    m_minimum_edit->setEnabled(m_boundary.limit_lower);
    m_lower_barrier_beta->setEnabled(m_boundary.limit_lower);
    m_lower_barrier_wall->setEnabled(m_boundary.limit_lower);

    m_maximum_edit->setEnabled(m_boundary.limit_upper);
    m_upper_barrier_beta->setEnabled(m_boundary.limit_upper);
    m_upper_barrier_wall->setEnabled(m_boundary.limit_upper);
    m_chart->setAutoScaleStrategy(AutoScaleStrategy::QtNiceNumbers);
    QJsonObject config = m_chart->getChartConfig();
    config["Legend"] = true;
    config["ScalingLocked"] = true;
    m_chart->setChartConfig(config);

    Recalculate();
    resize(800, 600);
}

void ParameterDialog::Adopt()
{
    m_value = (m_maximum_edit->value() + m_minimum_edit->value()) / 2.0;
    m_value_edit->setValue((m_maximum_edit->value() + m_minimum_edit->value()) / 2.0);
    accept();
}

void ParameterDialog::accept()
{
    m_value = m_value_edit->value();

    m_boundary.limit_lower = m_lower_limit->isChecked();
    m_boundary.limit_upper = m_upper_limit->isChecked();

    m_boundary.lower_barrier = m_minimum_edit->value();
    m_boundary.upper_barrier = m_maximum_edit->value();

    m_boundary.lower_barrier_beta = m_lower_barrier_beta->value();
    m_boundary.upper_barrier_beta = m_upper_barrier_beta->value();

    m_boundary.lower_barrier_wall = m_lower_barrier_wall->value();
    m_boundary.upper_barrier_wall = m_upper_barrier_wall->value();

    QDialog::accept();
}

void ParameterDialog::Recalculate()
{
    m_general_information->setText(QString("The current sum of squared errors (SSE) is %1.<\ br>The penalty function is the logfermi potential as given as in the following equation:<\ br>"
                                           "V = k * log(1 + exp( - %2*(x-x%3) )<\ br>"
                                           "With:\t x - the current parameter<\ br>"
                                           "\t x%3 - the boundary<\ br>"
                                           "\t k - the strength of potential<\ br>"
                                           "\t %2 - the steepness of the potential<\ br>"
                                           "More information for the logfermi potential can be found at the "
                                           "<a href='https://xtb-docs.readthedocs.io/en/latest/xcontrol.html\#different-potential-shapes'>xtb documentation of the Grimme Group</a>!")
                                       .arg(m_SSE)
                                       .arg(Unicode_beta)
                                       .arg(Unicode_Sub_0));

    double ymax = 4 * m_SSE;
    m_lowerbound->clear();
    m_upperbound->clear();
    m_combined->clear();
    m_sse_series->clear();
    m_current_value->clear();
    //   m_current_penalty->clear();
    double delta = 0;
    if (m_maximum_edit->value() - m_minimum_edit->value() < 1e-5)
        delta = 1;
    double difference = (m_maximum_edit->value() + delta) - (m_minimum_edit->value() - delta);
    double start = m_minimum_edit->value() - 2 * difference;
    double ende = m_maximum_edit->value() + 2 * difference;
    m_sse_series->append(start, m_SSE);
    m_sse_series->append(ende, m_SSE);

    for (double x = start; x <= ende; x += difference / 20.0) {
        double lower = LowerLogFermi(x, m_minimum_edit->value(), m_lower_barrier_wall->value(), m_lower_barrier_beta->value());
        double upper = UpperLogFermi(x, m_maximum_edit->value(), m_upper_barrier_wall->value(), m_upper_barrier_beta->value());
        m_lowerbound->append(x, lower);
        m_upperbound->append(x, upper);
        m_combined->append(x, lower + upper);
    }

    if (m_lower_limit->isChecked())
        m_left_boundary->setText(QString("Current lower penalty is %1.").arg(LowerLogFermi(m_value_edit->value(), m_minimum_edit->value(), m_lower_barrier_wall->value(), m_lower_barrier_beta->value())));
    else
        m_left_boundary->setText(QString("Current lower penalty would %1 be.").arg(LowerLogFermi(m_value_edit->value(), m_minimum_edit->value(), m_lower_barrier_wall->value(), m_lower_barrier_beta->value())));
    if (m_upper_limit->isChecked())
        m_right_boundary->setText(QString("Current upper penalty is %1.").arg(UpperLogFermi(m_value_edit->value(), m_maximum_edit->value(), m_lower_barrier_wall->value(), m_lower_barrier_beta->value())));
    else
        m_right_boundary->setText(QString("Current upper penalty would %1 be.").arg(UpperLogFermi(m_value_edit->value(), m_maximum_edit->value(), m_lower_barrier_wall->value(), m_lower_barrier_beta->value())));

    double tmp = LowerLogFermi(m_value_edit->value(), m_minimum_edit->value(), m_lower_barrier_wall->value(), m_lower_barrier_beta->value());
    if (!std::isinf(tmp))
        ymax = std::max(ymax, tmp);

    tmp = UpperLogFermi(m_value_edit->value(), m_maximum_edit->value(), m_lower_barrier_wall->value(), m_lower_barrier_beta->value());
    if (!std::isinf(tmp))
        ymax = std::max(ymax, tmp);

    m_current_value->append(m_value_edit->value(), 0);
    m_current_value->append(m_value_edit->value(), ymax);

    m_lowerbound->setVisible(m_lower_limit->isChecked());
    m_upperbound->setVisible(m_upper_limit->isChecked());
    m_combined->setVisible(m_lower_limit->isChecked() && m_upper_limit->isChecked());

    if (m_minimum_edit->value() <= m_value_edit->value() && m_value_edit->value() <= m_maximum_edit->value()) {
        m_value_edit->setStyleSheet("background-color: " + included());
        m_adopt_and_accept->hide();
    } else {
        m_value_edit->setStyleSheet("background-color: " + excluded());
        m_adopt_and_accept->show();
    }
    // m_chart->formatAxis();
    m_chart->setYRange(0, ymax);
    m_chart->setXRange(start, ende);
}
