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
#include "src/ui/widgets/optimizerwidget.h"
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QLabel>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFileDialog>

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



ConfigDialog::ConfigDialog(OptimizerConfig config, int printlevel,const QString &logfile, QWidget *parent) : m_opt_config(config), m_printlevel(printlevel), m_logfile(logfile), QDialog(parent)
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
    
    m_logfileButton = new QLineEdit;
    m_logfileButton->setClearButtonEnabled(true);
    m_logfileButton->setText(m_logfile);
    m_selectlogfile = new QPushButton(this);
    m_selectlogfile->setText(tr("Select file"));
    connect(m_selectlogfile, SIGNAL(clicked()), this, SLOT(SelectLogFile()));
    
    QHBoxLayout *h_layout = new QHBoxLayout;
    h_layout->addWidget(new QLabel(tr("Logfile:")));
    h_layout->addWidget(m_logfileButton);
    h_layout->addWidget(m_selectlogfile);
    layout->addLayout(h_layout);
    
    QLabel *printlevel = new QLabel(tr("Set Printlevel"));
    layout->addWidget(printlevel);
    
    m_printlevel_0 = new QRadioButton(tr("No Console Output (Printlevel 0)"), generalTab);
    if(m_printlevel == 0)
        m_printlevel_0->setChecked(true);
    layout->addWidget(m_printlevel_0);
    
    m_printlevel_1 = new QRadioButton(tr("Only Results (Printlevel 1)"), generalTab);
    if(m_printlevel == 1)
        m_printlevel_1->setChecked(true);
    layout->addWidget(m_printlevel_1);
    
    m_printlevel_2 = new QRadioButton(tr("Add Timing (Printlevel 2)"), generalTab);
    if(m_printlevel == 2)
        m_printlevel_2->setChecked(true);
    layout->addWidget(m_printlevel_2);
    
    m_printlevel_3 = new QRadioButton(tr("Add important intermediate Results (Printlevel 3)"), generalTab);    
    if(m_printlevel == 3)
        m_printlevel_3->setChecked(true);
    layout->addWidget(m_printlevel_3);
    
    m_printlevel_4 = new QRadioButton(tr("Add all intermediate Results (Printlevel 4)"), generalTab);
    if(m_printlevel == 4)
        m_printlevel_4->setChecked(true);
    layout->addWidget(m_printlevel_4);
    
    m_printlevel_5 = new QRadioButton(tr("Include debug information (Printlevel 5)"), generalTab);
    if(m_printlevel == 5)
        m_printlevel_5->setChecked(true);
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

void ConfigDialog::SelectLogFile()
{
    QString filename = QFileDialog::getSaveFileName(this, "Select file", ".");
    if(filename.isEmpty())
        return;
    
    m_logfileButton->setText(filename);
    m_logfile = filename;
    
}

void ConfigDialog::accept()
{
    if(m_printlevel_0->isChecked())
        m_printlevel = 0;    
    else if(m_printlevel_1->isChecked())
        m_printlevel = 1;    
    else if(m_printlevel_2->isChecked())
        m_printlevel = 2;       
    else if(m_printlevel_3->isChecked())
        m_printlevel = 3;  
    else if(m_printlevel_4->isChecked())
        m_printlevel = 4;  
    else
        m_printlevel = 5;  
        
   QDialog::accept(); 
}

#include "configdialog.moc"
