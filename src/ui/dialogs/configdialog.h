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

#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H
#include "src/ui/widgets/optimizerwidget.h"
#include <QDialog>
#include <QtWidgets/QLineEdit>

struct OptimizerConfig;

class QRadioButton;
class QTabWidget;
class QDialogButtonBox;


class OptimizerDialog : public QDialog
{
    Q_OBJECT

public:
    OptimizerDialog(OptimizerConfig config, QWidget *parent = 0);
    ~OptimizerDialog();
    OptimizerConfig Config() const { return m_opt_widget->Config();}
private:
    void setUi();
    void createOptimTab();
    QTabWidget *m_mainwidget;
    QDialogButtonBox *m_buttons;
    OptimizerConfig m_opt_config;
    OptimizerWidget *m_opt_widget;
    
};



class ConfigDialog : public QDialog
{
    Q_OBJECT

public:
    ConfigDialog(OptimizerConfig config, int printlevel, const QString &logfile, QWidget *parent = 0);
    ~ConfigDialog();
    inline OptimizerConfig Config() const { return m_opt_widget->Config();}
    inline QString LogFile() const {return m_logfile; }
    inline int PrintLevel() const { return m_printlevel; }
    virtual void accept();
private:
    QRadioButton *m_printlevel_0, *m_printlevel_1, *m_printlevel_2, *m_printlevel_3, *m_printlevel_4, *m_printlevel_5;
    QSpinBox *m_threads;
    QLineEdit *m_logfileButton;
    QPushButton *m_selectlogfile;
    QTabWidget *m_mainwidget;
    QDialogButtonBox *m_buttons;
    void setUi();
    void createGeneralTab();
    void createOptimTab();
    OptimizerConfig m_opt_config;
    OptimizerWidget *m_opt_widget;
    int m_printlevel;
    QString m_logfile;
private slots:
    void SelectLogFile(); 

    
};

#endif // CONFIGDIALOG_H
