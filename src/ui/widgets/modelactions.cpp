/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2017  Conrad Hübler <Conrad.Huebler@gmx.net>
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
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QWidget>
#include <QtWidgets/QPushButton>

#include "modelactions.h"

ModelActions::ModelActions(QWidget* parent) : QWidget(parent)
{
    setUi();
    resizeButtons();
}

ModelActions::~ModelActions()
{
}

void ModelActions::setUi()
{
    QHBoxLayout *layout = new QHBoxLayout;
    m_minimize_single = new PushButton(tr("Local Fits"));
    m_optim_config = new PushButton(tr("Fit Settings"));
    m_import = new PushButton(tr("Load Constants"));
    m_export = new PushButton(tr("Save Constants"));
    m_advanced = new PushButton(tr("Scan"));
    m_plot_3d = new PushButton(tr("3D Plot"));
    m_plot_3d->setEnabled(false);
    m_statistics = new PushButton(tr("Statistic"));
    m_concentration = new PushButton(tr("Concentration"));
    m_save = new PushButton(tr("Save"));
    m_new_guess = new PushButton(tr("New Guess"));
    
    connect(m_new_guess, SIGNAL(clicked()), this, SIGNAL(NewGuess()));
    connect(m_minimize_single, SIGNAL(clicked()), this, SIGNAL(LocalMinimize()));
    connect(m_optim_config, SIGNAL(clicked()), this, SIGNAL(OptimizerSettings()));
    connect(m_import, SIGNAL(clicked()), this, SIGNAL(ImportConstants()));
    connect(m_export, SIGNAL(clicked()), this, SIGNAL(ExportConstants()));
    connect(m_advanced, SIGNAL(clicked()), this, SIGNAL(OpenAdvancedSearch()));
    connect(m_plot_3d, SIGNAL(clicked()), this, SIGNAL(triggerPlot3D()));
    connect(m_statistics, SIGNAL(clicked()), this, SIGNAL(toggleStatisticDialog()));
    connect(m_save, SIGNAL(clicked()), this, SIGNAL(Save2File()));
    connect(m_concentration, SIGNAL(clicked()), this, SIGNAL(toggleConcentrations()));
    
    layout->addWidget(m_new_guess);
    layout->addWidget(m_minimize_single);
    layout->addWidget(m_advanced);
    //     mini->addWidget(m_plot_3d);
    layout->addWidget(m_optim_config);
    layout->addWidget(m_import);
    layout->addWidget(m_export);
    layout->addWidget(m_statistics);
    layout->addWidget(m_concentration);
    layout->addWidget(m_save); 
    setLayout(layout);
}

void ModelActions::resizeButtons()
{
    // Thats awfull and hackish, but it works for now and doesn't look that bad
    m_minimize_single->setMaximumSize(70, 30);
    m_optim_config->setMaximumSize(80, 30);
    m_import->setMaximumSize(110, 30);
    m_export->setMaximumSize(110, 30);
    m_advanced->setMaximumSize(50, 30);
    //     m_plot_3d->setMaximumSize(70, 30);
    m_statistics->setMaximumSize(70, 30);
    m_concentration->setMaximumSize(100, 30);
    m_save->setMaximumSize(70, 30);
    m_new_guess->setMaximumSize(80, 30);
}

#include "modelactions.moc"
