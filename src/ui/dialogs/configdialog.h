/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2016 - 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#pragma once

#include "src/ui/widgets/optimizerwidget.h"

#include <QtCore/QPointer>

#include <QtWidgets/QDialog>
#include <QtWidgets/QLineEdit>

struct OptimizerConfig;

class QRadioButton;
class QTabWidget;
class QDialogButtonBox;
class QComboBox;
class ScientificBox;

class OptimizerDialog : public QDialog {
    Q_OBJECT

public:
    OptimizerDialog(OptimizerConfig config, QWidget* parent = 0);
    ~OptimizerDialog();
    OptimizerConfig Config() const { return m_opt_widget->Config(); }

private:
    void setUi();
    void createOptimTab();
    QTabWidget* m_mainwidget;
    QDialogButtonBox* m_buttons;
    OptimizerConfig m_opt_config;
    OptimizerWidget* m_opt_widget;
};

class ConfigDialog : public QDialog {
    Q_OBJECT

public:
    ConfigDialog(OptimizerConfig config, QWidget* parent = 0);
    ~ConfigDialog();
    inline OptimizerConfig Config() const { return m_opt_widget->Config(); }
    virtual void accept();

private:
    QRadioButton *m_current_dir, *m_last_dir, *m_working_dir;
    QSpinBox *m_threads, *m_XScale, *m_YScale, *m_FontSize;
    QDoubleSpinBox *m_p_value, *m_markerSize, *m_chartScaling;
    QCheckBox *m_transparentChart, *m_cropedChart;
    QLineEdit* m_working;
    QPushButton* m_select_working;
    QTabWidget* m_mainwidget;
    QDialogButtonBox* m_buttons;
    QComboBox* m_charttheme;
    QCheckBox *m_animated_charts, *m_auto_confidence, *m_tooltips, *m_ask_on_exit, *m_save_on_exit, *m_series_confidence, *m_RemoveGrid, *m_EmphAxis;

    void setUi();
    void createGeneralTab();
    void createChartTab();
    void createStandardCalTab();
    void createOptimTab();

    OptimizerConfig m_opt_config;
    OptimizerWidget* m_opt_widget;
    int m_dirlevel;
    QString m_logfile, m_working_string;

private slots:
    void SelectWorkingDir();
};
