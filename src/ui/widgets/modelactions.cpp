/*
 * <one line to give the program's name and a brief idea of what it does.>
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

#include <QtCore/QTimer>

#include <QPropertyAnimation>

#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QWidget>

#include "modelactions.h"

ModelActions::ModelActions(QWidget* parent)
    : QWidget(parent)
    , m_hidden(true)
{
    setUi();
    resizeButtons();
}

ModelActions::~ModelActions()
{
}

void ModelActions::setUi()
{
    QVBoxLayout* layout = new QVBoxLayout;
    m_minimize_single = new PushButton(tr("Local Fits"));
    m_optim_config = new PushButton(tr("Fit Settings"));
    m_import = new PushButton(tr("Load Constants"));
    m_export = new PushButton(tr("Save Constants"));
    m_advanced = new PushButton(tr("Scan"));
    m_statistics = new PushButton(tr("Statistic"));
    m_save = new PushButton(tr("Save"));
    m_new_guess = new PushButton(tr("New Guess"));
    m_simulate = new PushButton(tr("Export Simulated"));
    m_plots = new PushButton(tr("Results"));
    //m_plots->setEnabled(false);
    m_detailed = new PushButton(tr("Detailed"));
    m_restore = new PushButton(tr("Restore"));
    m_charts = new PushButton(tr("Charts"));
    m_toggle = new QPushButton(tr("..more.."));
    m_toggle->setFlat(true);

    QHBoxLayout* h_layout = new QHBoxLayout;
    h_layout->addWidget(m_advanced);
    h_layout->addWidget(m_statistics);
    h_layout->addWidget(m_plots);
    h_layout->addWidget(m_detailed);
    h_layout->addWidget(m_new_guess);
    h_layout->addWidget(m_restore);
    h_layout->addWidget(m_charts);
    h_layout->addStretch();
    h_layout->addWidget(m_toggle);
    h_layout->setAlignment(Qt::AlignLeft);
    layout->addLayout(h_layout);

    m_second = new QWidget;
    layout->addWidget(m_second);

    h_layout = new QHBoxLayout;
    h_layout->addWidget(m_minimize_single);
    h_layout->addWidget(m_save);
    h_layout->addWidget(m_import);
    h_layout->addWidget(m_export);
    h_layout->addWidget(m_optim_config);
    h_layout->addStretch();
    h_layout->setAlignment(Qt::AlignLeft);
    m_second->setLayout(h_layout);

    connect(m_toggle, SIGNAL(clicked()), this, SLOT(ToggleMore()));
    connect(m_new_guess, SIGNAL(clicked()), this, SIGNAL(NewGuess()));
    connect(m_minimize_single, SIGNAL(clicked()), this, SIGNAL(LocalMinimize()));
    connect(m_optim_config, SIGNAL(clicked()), this, SIGNAL(OptimizerSettings()));
    connect(m_import, SIGNAL(clicked()), this, SIGNAL(ImportConstants()));
    connect(m_export, SIGNAL(clicked()), this, SIGNAL(ExportConstants()));
    connect(m_advanced, SIGNAL(clicked()), this, SIGNAL(OpenAdvancedSearch()));
    connect(m_statistics, SIGNAL(clicked()), this, SIGNAL(ToggleStatisticDialog()));
    connect(m_save, SIGNAL(clicked()), this, SIGNAL(Save2File()));
    connect(m_plots, SIGNAL(clicked()), this, SIGNAL(TogglePlot()));
    connect(m_restore, &PushButton::clicked, this, &ModelActions::Restore);
    connect(m_detailed, &PushButton::clicked, this, &ModelActions::Detailed);
    connect(m_charts, &PushButton::clicked, this, &ModelActions::Charts);

    m_second->setMaximumHeight(0);
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    setLayout(layout);
}

void ModelActions::EnableCharts(bool enable)
{
    //m_plots->setEnabled(enable);
}

void ModelActions::LocalEnabled(bool enabled)
{
    m_minimize_single->setEnabled(enabled);
}

void ModelActions::setSimulation(bool simulation)
{
    m_minimize_single->setHidden(simulation);
    m_restore->setHidden(simulation);
    m_minimize_single->setHidden(simulation);
    m_advanced->setHidden(simulation);
    m_optim_config->setHidden(simulation);
    m_advanced->setHidden(simulation);
    m_statistics->setHidden(simulation);
    m_plots->setHidden(simulation);
}

void ModelActions::resizeButtons()
{
    // Thats awfull and hackish, but it works for now and doesn't look that bad
    m_minimize_single->setMaximumSize(70, 30);
    m_charts->setMaximumSize(110, 30);
    m_optim_config->setMaximumSize(80, 30);
    m_import->setMaximumSize(110, 30);
    m_export->setMaximumSize(110, 30);
    m_advanced->setMaximumSize(50, 30);
    m_toggle->setMaximumSize(50, 30);
    m_plots->setMaximumSize(110, 30);
    m_statistics->setMaximumSize(70, 30);
    m_save->setMaximumSize(70, 30);
    m_new_guess->setMaximumSize(80, 30);
    m_detailed->setMaximumSize(120, 30);
    m_restore->setMaximumSize(120, 30);
}

void ModelActions::ToggleMore()
{

    if (!m_hidden) {
        QPropertyAnimation* animation = new QPropertyAnimation(m_second, "maximumHeight");
        animation->setEasingCurve(QEasingCurve::InOutCubic);
        animation->setDuration(200);
        animation->setStartValue(100);
        animation->setEndValue(0);
        animation->start();
        m_toggle->setText(tr("..more.."));
        m_hidden = true;
    } else {
        QPropertyAnimation* animation = new QPropertyAnimation(m_second, "maximumHeight");
        animation->setEasingCurve(QEasingCurve::InOutCubic);
        animation->setDuration(200);
        animation->setStartValue(0);
        animation->setEndValue(100);
        animation->start();
        m_toggle->setText(tr("..less.."));
        m_hidden = false;
    }
}

#include "modelactions.moc"
