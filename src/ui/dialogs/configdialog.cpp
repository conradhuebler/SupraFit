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

#include "src/ui/widgets/buttons/scientificbox.h"
#include "src/ui/widgets/optimizerwidget.h"

#include <QtCore/QThread>

#include <QApplication>

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QLabel>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFileDialog>

#include <QtCharts/QChart>

#include "configdialog.h"

OptimizerDialog::OptimizerDialog(OptimizerConfig config, QWidget *parent) :QDialog(parent), m_opt_config(config)
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
    
    m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok| QDialogButtonBox::Cancel);

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
    m_dirlevel = qApp->instance()->property("dirlevel").toInt();
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
    createStandardCalTab();
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
    
    m_threads = new QSpinBox;
    m_threads->setMaximum(QThread::idealThreadCount());
    m_threads->setValue(qApp->instance()->property("threads").toInt());
    m_threads->setMinimum(1);
    
    h_layout = new QHBoxLayout;
    h_layout->addWidget(new QLabel(tr("Threads:")));
    h_layout->addWidget(m_threads);
    layout->addLayout(h_layout);
    
    layout->addWidget(new QLabel(tr("Set directory behavior to:")));
    
    m_current_dir = new QRadioButton(tr("Current Directory, where Application was started"), generalTab);
    if(m_dirlevel == 0)
        m_current_dir->setChecked(true);
    layout->addWidget(m_current_dir);
    
    m_last_dir = new QRadioButton(tr("Last Directory"), generalTab);
    if(m_dirlevel == 1)
        m_last_dir->setChecked(true);
    layout->addWidget(m_last_dir);
    
    m_working_dir = new QRadioButton(tr("Set a working Directory"), generalTab);
    if(m_dirlevel == 2)
        m_working_dir->setChecked(true);
    layout->addWidget(m_working_dir);
    
    m_working = new QLineEdit;
    m_working->setClearButtonEnabled(true);
    m_working->setText(qApp->instance()->property("workingdir").toString());
    m_select_working = new QPushButton(this);
    m_select_working->setText(tr("Select"));
    connect(m_select_working, SIGNAL(clicked()), this, SLOT(SelectWorkingDir()));
    
    h_layout = new QHBoxLayout;
    h_layout->addWidget(new QLabel(tr("Working Directory:")));
    h_layout->addWidget(m_working);
    h_layout->addWidget(m_select_working);
    layout->addLayout(h_layout);
    
    layout->addWidget(new QLabel(tr("All changes below will only take effect after restarting the application\nor creating a new instance of anything being affected by that change.")));
    h_layout = new QHBoxLayout;
    h_layout->addWidget(new QLabel(tr("Chart Theme")));
    m_charttheme = createThemeBox();
    h_layout->addWidget(m_charttheme);
    m_charttheme->setCurrentIndex(qApp->instance()->property("charttheme").toInt());
    layout->addLayout(h_layout);
    
    
    m_animated_charts = new QCheckBox(tr("Animated Charts"));
    layout->addWidget(m_animated_charts);
    m_animated_charts->setChecked(qApp->instance()->property("chartanimation").toBool());
    /*QLabel *printlevel = new QLabel(tr("Set Printlevel"));
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
    layout->addWidget(m_printlevel_5);*/
    m_mainwidget->addTab(generalTab, tr("General Settings"));
}

void ConfigDialog::createStandardCalTab()
{
    QVBoxLayout *layout = new QVBoxLayout;
    QWidget *calcTab = new QWidget;
    calcTab->setLayout(layout);
    
    m_auto_confidence = new QCheckBox(tr("Automatic Confidence Calculation"));
    m_auto_confidence->setChecked(qApp->instance()->property("auto_confidence").toBool());
    layout->addWidget(m_auto_confidence);
    
    m_p_value = new QDoubleSpinBox;
    m_p_value->setMaximum(1);
    m_p_value->setMinimum(0);
    m_p_value->setSingleStep(1E-2);
    m_p_value->setValue(0.95);
    m_p_value->setDecimals(2);
    m_p_value->setValue(qApp->instance()->property("p_value").toDouble());
    
    QHBoxLayout *h_layout = new QHBoxLayout;
    h_layout->addWidget(new QLabel(tr("P Value:")));
    h_layout->addWidget(m_p_value);
    layout->addLayout(h_layout);
    
    m_mainwidget->addTab(calcTab, tr("Standard Calculation"));
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

void ConfigDialog::SelectWorkingDir()
{
    QString filename = QFileDialog::getExistingDirectory(this, "Choose Working Directory", qApp->instance()->property("workingdir").toString());
    if(filename.isEmpty())
        return;
    
    m_working->setText(filename);
}


void ConfigDialog::accept()
{
    if(m_current_dir->isChecked())
        m_dirlevel = 0;
    else if(m_last_dir->isChecked())
        m_dirlevel = 1;
    else if(m_working_dir->isChecked())
        m_dirlevel = 2;
    
    /*
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
        m_printlevel = 5;  */
   qApp->instance()->setProperty("charttheme", m_charttheme->itemData(m_charttheme->currentIndex()).toInt()); 
   qApp->instance()->setProperty("threads", m_threads->value()); 
   qApp->instance()->setProperty("chartanimation", m_animated_charts->isChecked());
   qApp->instance()->setProperty("workingdir", m_working->text());
   qApp->instance()->setProperty("dirlevel", m_dirlevel);
   qApp->instance()->setProperty("auto_confidence", m_auto_confidence->isChecked());
   qApp->instance()->setProperty("p_value", m_p_value->value());
   QDialog::accept(); 
}

QComboBox *ConfigDialog::createThemeBox() const
{
    QComboBox * themeComboBox = new QComboBox();
    themeComboBox->addItem("Light", QtCharts::QChart::ChartThemeLight);
    themeComboBox->addItem("Blue Cerulean", QtCharts::QChart::ChartThemeBlueCerulean);
    themeComboBox->addItem("Dark", QtCharts::QChart::ChartThemeDark);
    themeComboBox->addItem("Brown Sand", QtCharts::QChart::ChartThemeBrownSand);
    themeComboBox->addItem("Blue NCS", QtCharts::QChart::ChartThemeBlueNcs);
    themeComboBox->addItem("High Contrast", QtCharts::QChart::ChartThemeHighContrast);
    themeComboBox->addItem("Blue Icy", QtCharts::QChart::ChartThemeBlueIcy);
    themeComboBox->addItem("Qt", QtCharts::QChart::ChartThemeQt);
    return themeComboBox;
}

#include "configdialog.moc"
