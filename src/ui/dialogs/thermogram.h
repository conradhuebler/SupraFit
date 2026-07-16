/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2018 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <QtCore/QJsonObject>

#include <QtWidgets/QDialog>
#include <QtWidgets/QTabWidget>

#include <QtCharts/QChart>

#include "libpeakpick/peakpick.h"

#include "src/core/thermogramhandler.h"

class QCheckBox;
class QComboBox;
class QDialogButtonBox;
class QJsonObject;
class QLabel;
class QLineEdit;
class QDoubleSpinBox;
class QPushButton;
class QSplitter;
class QTableWidget;
class QTabWidget;

class ChartView;
class ItcProcessor;
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

    /*! \brief The one entry point for the cal->J factor: the processor re-scales both handlers, then
     * both combos are synced to show the same value. Claude Generated */
    void setScalingFactor(qreal factor);
    void setRaw(const QJsonObject& object);
    void setSystemParameter(const QJsonObject& object);
    void setRootDir(const QString& str) { m_root_dir = str; }
public slots:
    //virtual void reject() override();

private:
    void setUi();
    void UpdateTable();

    /*! \brief Set the status label to describe how the shown injection volumes were arrived at, and
     * flag a dilution that is shorter than the experiment. Claude Generated */
    void UpdateMessage(int injections);
    void ExportData();
    void ImportRow();
    void File2JsonBlock(const QString& filename, QJsonObject& block) const;

    /*! \brief Read an .itc file: returns its thermogram and the system parameters it recorded.
     *
     * Writes no members and touches no widgets - the caller decides what this file's contents mean.
     * A dilution run carries its own injection volumes and its own cell/syringe concentrations, and
     * neither of those is the titration's. `peaks` and `inject` are outputs describing this file.
     * Claude Generated */
    QPair<PeakPick::spectrum, QJsonObject> LoadITCFile(QString& filename, std::vector<PeakPick::Peak>* peaks, qreal& offset, QVector<qreal>& inject);

    /*! \brief Adopt `parameter` as the titration's system parameters and show them in the fields.
     *
     * Only the experiment's are the titration's; see LoadITCFile. Claude Generated */
    void ApplySystemParameter(const QJsonObject& parameter);

    /*! \brief The four import columns per injection: volume [uL], raw experiment heat, raw dilution
     * heat, net heat [J].
     *
     * The single place that knows the column layout - the table, the charts and the export all
     * render this. Columns 0 and 3 come from the ItcProcessor, which owns the volumes and the
     * experiment-minus-dilution join; 1 and 2 are unscaled per-handler diagnostics read straight
     * off the handlers, since the join has no use for them. Claude Generated */
    QVector<QVector<qreal>> ResultRows() const;
    PeakPick::spectrum LoadXYFile(const QString& filename);

    QPushButton *m_exp_button, *m_dil_button, *m_refit, *m_export_data, *m_import_row;
    QCheckBox *m_remove_offset, *m_UseParameter, *m_constantVolume, *m_showDilution, *m_uniformInject;
    QLineEdit *m_exp_file, *m_dil_file, *m_injct, *m_exp_base, *m_dil_base, *m_CellVolume, *m_CellConcentration, *m_SyringeConcentration, *m_Temperature;
    QComboBox* m_scale;
    QLabel* m_message;
    QTabWidget* m_mainwidget;
    QDoubleSpinBox* m_freq;
    QTableWidget* m_table;
    ThermogramWidget *m_experiment, *m_dilution;
    ItcProcessor* m_processor; //!< owns the two handlers + injection volumes + exp-minus-dilution join
    ThermogramHandler *m_experiment_thermogram, *m_dilution_thermogram; //!< non-owning, point into m_processor

    ChartView* m_data_view;
    QSplitter* m_splitter;

    std::vector<PeakPick::Peak> m_exp_peaks, m_dil_peaks;
    ScatterSeries *m_thm_series, *m_raw_series, *m_dil_series;
    QDialogButtonBox* m_buttonbox;

    QString m_root_dir;
    QJsonObject m_systemparameter;
    bool m_ParameterUsed = false;
    bool m_updating_table = false; //!< guards UpdateTable() rebuilds against the cellChanged handler (Claude Generated)

private slots:
    void setExperiment();
    void clearExperiment();
    void setDilution();
    void clearDilution();
    void UpdateData();

    /*! \brief Bring the processor's volume vector in line with the inject field and the uniform
     * checkbox, before the table is rendered.
     *
     * Checked: broadcast the field's value to every injection. Unchecked: keep the per-injection
     * volumes (from the file or manual edits) and only pad any rows the vector does not yet cover.
     * Resolving here, rather than at render time, is what lets the volumes be stored and exported as
     * exactly what the table shows. Claude Generated */
    void ResolveInjectionVolumes();
};
