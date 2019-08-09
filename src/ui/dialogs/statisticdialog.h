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

#pragma once

#include <QtCore/QMutex>
#include <QtCore/QTime>

#include <QtGui/QPainter>
#include <QtGui/QTextDocument>

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QStyledItemDelegate>

#include "src/capabilities/resampleanalyse.h"

class QCheckBox;
class QGroupBox;
class QDoubleSpinBox;
class QLabel;
class QSpinBox;
class QProgressBar;
class QPushButton;
class QRadioButton;
class QTabWidget;

class ScientificBox;

class WGSConfig;
class MCConfig;
class MoCoConfig;

class RadioButton : public QRadioButton {
    Q_OBJECT

public:
    RadioButton(const QString& str)
        : QRadioButton(str)
    {
    }

protected:
    void paint(QPainter* painter) const
    {
        /*
        QStyleOptionViewItemV4 options = option;
        initStyleOption();
        */
        painter->save();

        QTextDocument doc;
        doc.setHtml(text());

        /* options.text = "";
        options.widget->style()->drawControl(QStyle::CE_ItemViewItem, &options, painter);
        options.widget->style()->drawControl(QStyle::CE_ComboBoxLabel, &options, painter);

        painter->translate(options.rect.left(), options.rect.top());*/
        QRect clip(0, 0, width(), height());
        doc.drawContents(painter, clip);

        painter->restore();
    }

    QSize sizeHint() const override
    {
        /*
        QStyleOptionViewItemV4 options = option;
        initStyleOption(&options);
        */
        QTextDocument doc;
        doc.setHtml(text());
        return QSize(doc.idealWidth(), doc.size().height());
    }
};

class StatisticDialog : public QDialog {
    Q_OBJECT

public:
    StatisticDialog(QSharedPointer<AbstractModel> m_model, QWidget* parent = 0);
    StatisticDialog(QWidget* parent = 0);
    virtual ~StatisticDialog() override;

    QJsonObject RunMonteCarlo() const;
    QJsonObject RunGridSearch() const;
    QJsonObject RunModelComparison() const;
    QJsonObject RunReductionAnalyse() const;
    QJsonObject RunCrossValidation() const;

    inline void setRuns(int runs) { m_main_steps = runs; }
    virtual void setVisible(bool visible) override;
    inline bool UseChecked() const { return m_use_checked->isChecked(); }
    inline bool isMCStd() const { return m_mc_std->isChecked(); }
    inline bool isMCSEy() const { return m_mc_sey->isChecked(); }
    inline bool isMCUser() const { return m_mc_user->isChecked(); }
    inline qreal MCStd() const { return m_varianz_box->value(); }

    void updateUI();

public slots:
    void MaximumSteps(int steps);
    void MaximumMainSteps(int steps);
    void IncrementProgress(int time);
    void IncrementMainProgress();
    void HideWidget();
    void ShowWidget();
    void Attention();
    void Message(const QString& str);

private:
    void setUi();
    void clearMessages();
    QString FOutput() const;

    QWidget* MonteCarloWidget();
    QWidget* GridSearchWidget();
    QWidget* ModelComparison();
    QWidget* CVWidget();

    QWidget *m_hide_widget, *m_moco_widget;
    QTabWidget* m_tab_widget;
    QDoubleSpinBox *m_varianz_box, *m_wgs_increment, *m_wgs_maxerror, *m_moco_maxerror, *m_moco_box_multi, *m_moco_f_value, *m_wgs_f_value;
    ScientificBox* m_wgs_err_conv;
    QSpinBox *m_cv_runs, *m_cv_lxo, *m_mc_steps, *m_wgs_steps, *m_moco_mc_steps, *m_gridOvershotCounter, *m_gridErrorDecreaseCounter, *m_gridErrorConvergencyCounter, *m_gridScalingFactor;
    QCheckBox *m_original, *m_use_checked, *m_store_wgsearch;
    QVector<QCheckBox*> m_indepdent_checkboxes, m_grid_global, m_grid_local, m_moco_global, m_moco_local;
    QVector<QDoubleSpinBox*> m_indepdent_variance, m_glob_box_scaling, m_loc_box_scaling;
    QVector<QSpinBox*> m_global_moco_digits, m_local_moco_digits;
    QPushButton *m_mc, *m_cv, *m_interrupt, *m_hide, *m_moco, *m_cross_validate, *m_reduction;
    QGroupBox *m_moco_global_settings, *m_moco_monte_carlo;
    QProgressBar *m_progress, *m_main_progress;
    QLabel *m_time_info, *m_wgs_error_info, *m_moco_error_info, *m_message_box;
    QRadioButton *m_wgs_loo, *m_wgs_l2o, *m_wgs_lxo;
    QRadioButton *m_mc_std, *m_mc_sey, *m_mc_user, *m_mc_bootstrap;
    QRadioButton *m_cv_premap, *m_cv_automap, *m_cv_randmap;
    QMutex mutex;

    QWeakPointer<AbstractModel> m_model;

    int m_time, m_main_steps = 0;
    qint64 m_time_0;
    qreal m_f_value = 1, m_moco_max, m_wgs_max;
    bool m_hidden;

private slots:
    void Update();
    void CalculateError();

signals:
    void RunCalculation(const QJsonObject& controller);
    void Reduction();
    void Interrupt();
    void setMaximumSteps(int steps);
};
