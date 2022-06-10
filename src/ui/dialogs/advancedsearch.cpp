/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2017 - 2019 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/capabilities/globalsearch.h"

#include "src/core/models/AbstractModel.h"

#include "src/core/minimizer.h"
#include "src/core/toolset.h"

#include "src/ui/guitools/waiter.h"

#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <QtCore/QJsonObject>
#include <QtCore/QMutexLocker>
#include <QtCore/QPointer>
#include <QtCore/QThreadPool>
#include <QtCore/QTimer>

#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QTabWidget>

#include <iostream>

#include "advancedsearch.h"

ParameterWidget::ParameterWidget(const QString& name, qreal value, QWidget* parent)
    : QGroupBox(parent)
    , m_value(value)
    , m_name(name)
{
    qreal div = value / 5.0;
    qreal prod = value * 5;
    m_min = new QDoubleSpinBox;
    m_min->setMinimum(-1e8);
    m_min->setMaximum(1e8);
    m_min->setValue(ToolSet::floor(qMin(div, prod)));

    m_max = new QDoubleSpinBox;
    m_max->setMinimum(-1e8);
    m_max->setMaximum(1e8);
    m_max->setValue(ToolSet::ceil(qMax(div, prod)));

    m_step = new QDoubleSpinBox;
    m_step->setMinimum(0.01);
    m_step->setMaximum(1e8);
    m_step->setValue(ToolSet::ceil((m_max->value() - m_min->value()) / 10));

    m_variable = new QCheckBox(tr("Include in variation"));
    m_variable->setToolTip(tr("If checked, the parameter will be varied according from the start to the end.\nIf unchecked, the current value will be taken."));
    m_variable->setChecked(true);
    connect(m_variable, &QCheckBox::stateChanged, m_step, &QDoubleSpinBox::setEnabled);
    connect(m_variable, &QCheckBox::stateChanged, m_max, &QDoubleSpinBox::setEnabled);
    connect(m_variable, &QCheckBox::stateChanged, m_min, &QDoubleSpinBox::setEnabled);
    connect(m_variable, &QCheckBox::stateChanged, this, &ParameterWidget::valueChanged);

    m_optimise = new QCheckBox(tr("Optimise Parameter"));
    m_optimise->setToolTip(tr("If checked, this parameter will be optimised during fitting process.\nIf unchecked, the parameter will be fixed during fitting, but not necessarily in global search process."));
    m_optimise->setChecked(true);
    connect(m_optimise, &QCheckBox::stateChanged, this, &ParameterWidget::checkChanged);

    connect(m_min, SIGNAL(valueChanged(double)), this, SIGNAL(valueChanged()));
    connect(m_max, SIGNAL(valueChanged(double)), this, SIGNAL(valueChanged()));
    connect(m_step, SIGNAL(valueChanged(double)), this, SIGNAL(valueChanged()));

    QGridLayout* layout = new QGridLayout;

    m_info = new QLabel(tr("<h4>Parameter: %1 (%2)</h4>").arg(m_name).arg(m_value));
    layout->addWidget(m_info, 0, 0, 1, 6);

    layout->addWidget(new QLabel(tr("Start:")), 1, 0);
    layout->addWidget(m_min, 1, 1);
    layout->addWidget(new QLabel(tr("End:")), 1, 2);
    layout->addWidget(m_max, 1, 3);
    layout->addWidget(new QLabel(tr("Step")), 1, 4);
    layout->addWidget(m_step, 1, 5);
    layout->addWidget(m_variable,2,0,1,3);
    layout->addWidget(m_optimise, 2, 4, 1, 3);
    setLayout(layout);
}

void ParameterWidget::setValue(qreal value)
{
    m_value = value;
    m_info->setText(tr("<h4>Parameter: %1 (%2)</h4>").arg(m_name).arg(m_value));
}

AdvancedSearch::AdvancedSearch(QWidget* parent)
    : QDialog(parent)
{
    setModal(false);
    m_error_max = 0;
    setWindowTitle("Global Search Dialog");
}

AdvancedSearch::~AdvancedSearch()
{
    m_model.clear();
}

double AdvancedSearch::MaxX() const
{
    if (m_model.toStrongRef()->GlobalParameterSize() >= 1)
        return m_parameter_list[0]->Max();
    return 0;
}

double AdvancedSearch::MinX() const
{
    if (m_model.toStrongRef()->GlobalParameterSize() >= 1)
        return m_parameter_list[0]->Min();
    return 0;
}

double AdvancedSearch::MaxY() const
{
    if (m_model.toStrongRef()->GlobalParameterSize() >= 2)
        return m_parameter_list[1]->Max();
    return 0;
}

double AdvancedSearch::MinY() const
{
    if (m_model.toStrongRef()->GlobalParameterSize() >= 2)
        return m_parameter_list[1]->Min();
    return 0;
}

void AdvancedSearch::SetUi()
{
    if (m_model.isNull())
        return;

    m_search = new GlobalSearch(this);

    QVBoxLayout* layout = new QVBoxLayout;

    for (int i = 0; i < m_model.toStrongRef()->GlobalParameterSize(); ++i) {
        QPointer<ParameterWidget> widget = new ParameterWidget(m_model.toStrongRef()->GlobalParameterName(i), m_model.toStrongRef()->GlobalParameter(i), this);
        layout->addWidget(widget);
        widget->setEnabled(m_model.toStrongRef()->GlobalEnabled(i));
        connect(widget, SIGNAL(valueChanged()), this, SLOT(MaxSteps()));

        connect(m_model.toStrongRef().data(), &AbstractModel::Recalculated, widget, [widget, i, this]() {
            widget->setEnabled(m_model.toStrongRef()->GlobalEnabled(i));
            widget->setValue(m_model.toStrongRef()->GlobalParameter(i));
        });

        connect(widget, &ParameterWidget::checkChanged, m_model.toStrongRef().data(), [this, i](int state) {
            m_model.toStrongRef()->GlobalTable()->setChecked(0, i, state);
        });

        m_parameter_list << widget;
    }

    if (!m_model.toStrongRef()->SupportSeries()) {
        for (int i = 0; i < m_model.toStrongRef()->LocalParameterSize(); ++i) {
            QPointer<ParameterWidget> widget = new ParameterWidget(m_model.toStrongRef()->LocalParameterName(i), m_model.toStrongRef()->LocalParameter(i, 0), this);
            layout->addWidget(widget);
            widget->setEnabled(m_model.toStrongRef()->LocalEnabled(i));
            connect(widget, SIGNAL(valueChanged()), this, SLOT(MaxSteps()));

            connect(m_model.toStrongRef().data(), &AbstractModel::Recalculated, widget, [widget, i, this]() {
                widget->setEnabled(m_model.toStrongRef()->LocalEnabled(i));
                widget->setValue(m_model.toStrongRef()->LocalParameter(i, 0));
            });

            connect(widget, &ParameterWidget::checkChanged, m_model.toStrongRef().data(), [this, i](int state) {
                m_model.toStrongRef()->LocalTable()->setChecked(0, i, state);
            });

            m_parameter_list << widget;
        }
    } else {
        QTabWidget* tabwidget = new QTabWidget;
        layout->addWidget(new QLabel("<h4>Series and Local Parameter</h4>"));
        layout->addWidget(tabwidget);

        for (int j = 0; j < m_model.toStrongRef()->SeriesCount(); ++j) {
            QWidget* w = new QWidget;
            QVBoxLayout* vlayout = new QVBoxLayout;
            w->setLayout(vlayout);
            for (int i = 0; i < m_model.toStrongRef()->LocalParameterSize(j); ++i) {
                QPointer<ParameterWidget> widget = new ParameterWidget(m_model.toStrongRef()->LocalParameterName(i), m_model.toStrongRef()->LocalParameter(i, j), this);
                widget->Disable(true);
                vlayout->addWidget(widget);
                widget->setEnabled(m_model.toStrongRef()->LocalEnabled(i));
                connect(widget, SIGNAL(valueChanged()), this, SLOT(MaxSteps()));

                connect(m_model.toStrongRef().data(), &AbstractModel::Recalculated, widget, [widget, i, this, j]() {
                    widget->setEnabled(m_model.toStrongRef()->LocalEnabled(i));
                    widget->setValue(m_model.toStrongRef()->LocalParameter(i, j));
                });

                connect(widget, &ParameterWidget::checkChanged, m_model.toStrongRef().data(), [this, i, j](int state) {
                    m_model.toStrongRef()->LocalTable()->setChecked(j, i, state);
                });

                m_parameter_list << widget;
            }
            tabwidget->addTab(w, tr("Series No %1").arg(j + 1));
        }
    }

    m_scan = new QPushButton(tr("Scan"));
    m_interrupt = new QPushButton(tr("Interrupt"));

    connect(m_scan, SIGNAL(clicked()), this, SLOT(SearchGlobal()));

    connect(m_interrupt, &QPushButton::clicked, this, &AdvancedSearch::Interrupt, Qt::DirectConnection);
    m_progress = new QProgressBar;
    m_max_steps = new QLabel;
    QGridLayout* mlayout = new QGridLayout;
    QScrollArea* scroll = new QScrollArea;
    scroll->setFixedWidth(560);
    QWidget* scrollWidget = new QWidget;
    scrollWidget->setLayout(layout);
    scroll->setWidget(scrollWidget);
    mlayout->addWidget(scroll, 0, 0, 1, 2);
    mlayout->addWidget(m_max_steps, 1, 0, 1, 2);
    mlayout->addWidget(m_progress, 2, 0, 1, 2);

    mlayout->addWidget(m_scan, 3, 0);
    mlayout->addWidget(m_interrupt, 3, 1);

    m_interrupt->hide();
    connect(this, SIGNAL(setValue(int)), m_progress, SLOT(setValue(int)));

    setLayout(mlayout);
}


void AdvancedSearch::MaxSteps()
{
    m_parameter.clear();
    m_ignored_parameter.clear();
    int max_count = 1;
    for (int i = 0; i < m_parameter_list.size(); ++i) {
        double min = m_parameter_list[i]->Value(), max = m_parameter_list[i]->Value(), step = 1, optimise = m_parameter_list[i]->Optimise();
        if(m_parameter_list[i]->Variable())
        {
            if (!m_parameter_list[i]->isEnabled()) {
                min = m_parameter_list[i]->Value();
                max = m_parameter_list[i]->Value();
            } else {
                min = m_parameter_list[i]->Min();
                max = m_parameter_list[i]->Max();
                step = m_parameter_list[i]->Step();
            }
        }
        m_parameter.append(QVector<qreal>() << min << max << step << optimise);

        if (max > min)
            max_count *= std::ceil((max - min) / step);
    }
    m_max_steps->setText(tr("No of calculations to be done: %1").arg(max_count));
}

void AdvancedSearch::HideWidget()
{
    m_scan->show();
    m_interrupt->hide();
    hide();
}

void AdvancedSearch::SearchGlobal()
{
    m_interrupt->show();
    m_scan->hide();
    MaxSteps();
    QJsonObject job;

    job["ParameterSize"] = m_parameter.size();
    job["Method"] = SupraFit::Method::GlobalSearch;
    for (int i = 0; i < m_parameter.size(); ++i) {
        job[QString::number(i)] = ToolSet::DoubleVec2String(m_parameter[i]);
    }
    emit RunCalculation(job);
}

void AdvancedSearch::IncrementProgress(int time)
{
    QMutexLocker locker(&mutex);
    m_time += time;
    qint64 t0 = QDateTime::currentMSecsSinceEpoch();
    int val = m_progress->value() + 1;
    qreal aver = double(m_time) / val;
    int remain = double(m_progress->maximum() - val) * aver / 3000;
    int used = (t0 - m_time_0) / 1000;
    m_max_steps->setText(tr("Remaining time approx: %1 sec., elapsed time: %2 sec. .").arg(remain).arg(used));
    emit setValue(val);
}

void AdvancedSearch::MaximumSteps(int steps)
{
    m_time = 0;
    m_time_0 = QDateTime::currentMSecsSinceEpoch();
    m_progress->setValue(0);
    m_progress->setMaximum(steps);
}

#include "advancedsearch.moc"
