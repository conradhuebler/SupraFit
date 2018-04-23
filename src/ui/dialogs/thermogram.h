/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <Eigen/Dense>

#include <QtWidgets/QDialog>

#include <QtCharts/QChart>

#include "libpeakpick/peakpick.h"

class QDialogButtonBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;
class QTabWidget;

class ChartView;
class LineSeries;
class ScatterSeries;

typedef Eigen::VectorXd Vector;

class Thermogram : public QDialog {
    Q_OBJECT
public:
    Thermogram();

    QString Content(); //{ return m_content; }
    virtual void keyPressEvent(QKeyEvent* evt) override
    {
        if (evt->key() == Qt::Key_Enter || evt->key() == Qt::Key_Return)
            return;
        QDialog::keyPressEvent(evt);
    }

    void setExperimentFile(const QString& str);

private:
    void setUi();
    void UpdateTable();

    PeakPick::spectrum LoadITCFile(const QString& filename, std::vector<PeakPick::Peak>* peaks);
    PeakPick::spectrum LoadXYFile(const QString& filename);

    QPushButton *m_exp_button, *m_dil_button;
    QLineEdit *m_exp_file, *m_dil_file, *m_injct;
    QLabel* m_message;
    QTabWidget* m_mainwidget;
    QTableWidget *m_table, *m_exp_table, *m_dil_table;
    ChartView *m_experiment_view, *m_dilution_view, *m_thermogram_view;
    QtCharts::QChart *m_experiment, *m_dilution, *m_therm;
    LineSeries* fromSpectrum(const PeakPick::spectrum original);

    std::vector<PeakPick::Peak> PickPeaks(const PeakPick::spectrum, QTableWidget* widget);
    std::vector<PeakPick::Peak> m_exp_peaks, m_dil_peaks;
    ScatterSeries* m_thermogram;
    QDialogButtonBox* m_buttonbox;

    QString m_content;
    QVector<qreal> m_heat, m_raw, m_inject;
    bool m_forceInject = false, m_injection = false;

private slots:
    void setExperiment();
    void setDilution();
    void UpdateInject();
};
