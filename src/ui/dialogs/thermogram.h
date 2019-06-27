/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2018 - 2019 Conrad Hübler <Conrad.Huebler@gmx.net>
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
#include <QtWidgets/QTabWidget>

#include <QtCharts/QChart>

#include "libpeakpick/peakpick.h"

class QCheckBox;
class QComboBox;
class QDialogButtonBox;
class QLabel;
class QLineEdit;
class QDoubleSpinBox;
class QPushButton;
class QSplitter;
class QTableWidget;
class QTabWidget;

class ChartView;
class LineSeries;
class ScatterSeries;
class ThermogramWidget;

typedef Eigen::VectorXd Vector;

class Thermogram : public QDialog {
    Q_OBJECT
public:
    Thermogram();
    virtual ~Thermogram();

    QString Content() const;
    inline QJsonObject SystemParamter() const { return m_systemparameter; }
    inline bool ParameterUsed() const { return m_ParameterUsed; }

    QJsonObject Raw() const;
    QString ProjectName() const;

    virtual void keyPressEvent(QKeyEvent* event) override
    {
        if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return)
            return;
        if (event->key() == Qt::Key_Escape) {
            /* Let ESC only work, if we are not on any thermogram widget, thus that ESC only acts on the chartview */
            if (m_mainwidget->currentIndex())
                return;
        }
        QDialog::keyPressEvent(event);
    }

    void setExperimentFile(QString str);
    void setDilutionFile(QString str);
    void setExperimentFit(const QJsonObject& json);
    void setDilutionFit(const QJsonObject& json);
    void setScaling(const QString& str);
    void setRaw(const QJsonObject& object);

public slots:
    //virtual void reject() override();

private:
    void setUi();
    void UpdateTable();
    void UpdateExpTable();
    void UpdateDilTable();
    void ExportData();

    PeakPick::spectrum LoadITCFile(QString& filename, std::vector<PeakPick::Peak>* peaks, qreal& offset);
    PeakPick::spectrum LoadXYFile(const QString& filename);

    QPushButton *m_exp_button, *m_dil_button, *m_refit, *m_export_data;
    QCheckBox *m_remove_offset, *m_UseParameter, *m_constantVolume;
    QLineEdit *m_exp_file, *m_dil_file, *m_injct, *m_exp_base, *m_dil_base, *m_CellVolume, *m_CellConcentration, *m_SyringeConcentration, *m_Temperature;
    QComboBox* m_scale;
    QLabel *m_message, *m_offset;
    QTabWidget* m_mainwidget;
    QDoubleSpinBox* m_freq;
    QTableWidget* m_table;
    ThermogramWidget *m_experiment, *m_dilution;
    ChartView* m_data_view;
    QSplitter* m_splitter;
    // QtCharts::QChart* m_data;
    LineSeries* fromSpectrum(const PeakPick::spectrum original);

    std::vector<PeakPick::Peak> PickPeaks(const PeakPick::spectrum, QTableWidget* widget);
    std::vector<PeakPick::Peak> m_exp_peaks, m_dil_peaks;
    PeakPick::spectrum m_exp_therm, m_dil_therm;
    ScatterSeries *m_thm_series, *m_raw_series, *m_dil_series;
    QDialogButtonBox* m_buttonbox;

    QString m_content, m_all_rows;
    QJsonObject m_systemparameter;
    QVector<qreal> m_heat, m_raw, m_dil_heat, m_inject;
    bool m_forceInject = false, m_injection = false, m_forceStep = false, m_ParameterUsed = false;
    qreal m_heat_offset = 0, m_dil_offset = 0;
    qreal PeakAt(int i);

private slots:
    void setExperiment();
    void clearExperiment();
    void setDilution();
    void clearDilution();
    void UpdateData();
};
