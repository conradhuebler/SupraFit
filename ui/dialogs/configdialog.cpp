/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2016  Conrad Hübler <Conrad.Huebler@gmx.net>
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
#include "ui/widgets/optimizerwidget.h"

#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QLabel>
#include <QtWidgets/QDialogButtonBox>
#include "configdialog.h"


OptimizerDialog::OptimizerDialog(OptimizerConfig config, QWidget *parent) : m_opt_config(config), QDialog(parent)
{
    
    setUi();
    
}

OptimizerDialog::~OptimizerDialog()
{
    
    
    
    
}


void OptimizerDialog::setUi()
{
    QVBoxLayout *mainlayout = new QVBoxLayout;
    
    setLayout(mainlayout);
    
    m_mainwidget = new QTabWidget;
    mainlayout->addWidget(m_mainwidget);
    
    createOptimTab();
    
    m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok
                                     | QDialogButtonBox::Cancel);

    connect(m_buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    
    mainlayout->addWidget(m_buttons);
    
}

void OptimizerDialog::createOptimTab()
{
    QVBoxLayout *layout = new QVBoxLayout;
    m_opt_widget = new OptimizerWidget(m_opt_config, this);
    m_opt_widget->setLayout(layout);
    
    m_mainwidget->addTab(m_opt_widget, tr("Optimizer Settings"));
}



ConfigDialog::ConfigDialog(OptimizerConfig config, QWidget *parent) : m_opt_config(config), QDialog(parent)
{
    
    setUi();
    
}

ConfigDialog::~ConfigDialog()
{
    
    
    
    
}


void ConfigDialog::setUi()
{
    QVBoxLayout *mainlayout = new QVBoxLayout;
    
    setLayout(mainlayout);
    
    m_mainwidget = new QTabWidget;
    mainlayout->addWidget(m_mainwidget);
    
    createGeneralTab();
    createOptimTab();
    
    m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok
                                     | QDialogButtonBox::Cancel);

    connect(m_buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    
    mainlayout->addWidget(m_buttons);
    
}

void ConfigDialog::createGeneralTab()
{
    QVBoxLayout *layout = new QVBoxLayout;
    QWidget *generalTab = new QWidget;
    generalTab->setLayout(layout);
    
    QLabel *printlevel = new QLabel(tr("Set Printlevel"));
    layout->addWidget(printlevel);
    m_printlevel_0 = new QRadioButton(tr("No Console Output (Printlevel 0)"), generalTab);
    layout->addWidget(m_printlevel_0);
    m_printlevel_1 = new QRadioButton(tr("Only Results (Printlevel 1)"), generalTab);
    layout->addWidget(m_printlevel_1);
    m_printlevel_2 = new QRadioButton(tr("Add Timing (Printlevel 2)"), generalTab);
    layout->addWidget(m_printlevel_2);
    m_printlevel_3 = new QRadioButton(tr("Add important intermediate Results (Printlevel 3)"), generalTab);
    layout->addWidget(m_printlevel_3);
    m_printlevel_4 = new QRadioButton(tr("Add all intermediate Results (Printlevel 4)"), generalTab);
    layout->addWidget(m_printlevel_4);
    m_printlevel_5 = new QRadioButton(tr("Include debug information (Printlevel 5)"), generalTab);
    layout->addWidget(m_printlevel_5);
    m_mainwidget->addTab(generalTab, tr("General Settings"));
}


void ConfigDialog::createOptimTab()
{
    QVBoxLayout *layout = new QVBoxLayout;
    m_opt_widget = new OptimizerWidget(m_opt_config, this);
    m_opt_widget->setLayout(layout);
    
    m_mainwidget->addTab(m_opt_widget, tr("Optimizer Settings"));
}

#include "configdialog.moc"
