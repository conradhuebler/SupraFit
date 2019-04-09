/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2017 - 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/capabilities/globalsearch.h"

#include "src/core/AbstractModel.h"
#include "src/global.h"

#include <QtCore/QMutex>
#include <QtCore/QPointer>
#include <QtCore/QWeakPointer>

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QGroupBox>

class QLabel;
class QLineEdit;
class Minimizer;
class QCheckBox;
class QPushButton;
class QDoubleSpinBox;
class QJsonObject;
class OptimizerFlagWidget;
class QProgressBar;

struct GlobalSearchResult;

class ParameterWidget : public QGroupBox {
    Q_OBJECT

public:
    ParameterWidget(const QString& name, qreal value, QWidget* parent = 0);
    inline ~ParameterWidget() {}
    inline double Min() const { return m_min->value(); }
    inline double Max() const { return m_max->value(); }
    inline double Step() const {  return m_step->value(); }
    inline bool Optimise() const { return m_optimise->isChecked(); }
    inline bool Variable() const { return m_variable->isChecked(); }
    inline qreal Value() const { return m_value; }
    inline void Disable(bool disable)
    {
        m_optimise->setChecked(disable);
        m_variable->setChecked(!disable);
    }
    void setValue(qreal value);

private:
    QPointer<QDoubleSpinBox> m_min, m_max, m_step;
    QPointer<QCheckBox> m_variable, m_optimise;
    qreal m_value;
    QLabel* m_info;
    QString m_name;

signals:
    void valueChanged();
    void checkChanged(int state);
};

class AdvancedSearch : public QDialog {
    Q_OBJECT

public:
    AdvancedSearch(QWidget* parent = 0);
    ~AdvancedSearch();

    inline void setModel(const QSharedPointer<AbstractModel> model)
    {
        m_model = model;
        SetUi();
    }
    inline double MaxError() const { return m_error_max; }
    double MaxX() const;
    double MinX() const;
    double MaxY() const;
    double MinY() const;
    QList<QList<QPointF>> Series() const { return m_series; }
    QList<QJsonObject> ModelList() const { return m_models_list; }
    QPointer<GlobalSearch> globalSearch() const { return m_search; }
    //GSConfig Config() const;

private:
    void SetUi();
    void Scan(const QVector<QVector<double>>& list);

    QProgressBar* m_progress;

    QWeakPointer<AbstractModel> m_model;
    QCheckBox *m_optim, *m_initial_guess;
    QPointer<QPushButton> m_scan, m_interrupt;
    QLabel* m_max_steps;
    void ConvertList(const QVector<QVector<double>>& list, QVector<double>& error);
    QList<QList<QPointF>> m_series;
    double m_error_max;
    QVector<QVector<double>> ParamList();
    QList<QJsonObject> m_models_list;
    QVector<QPointer<ParameterWidget>> m_parameter_list;
    QMutex mutex;
    int m_time;
    quint64 m_time_0;
    QVector<QVector<qreal>> m_parameter;
    QPointer<GlobalSearch> m_search;
    QList<int> m_ignored_parameter;

    void PrepareProgress();
    void Finished();

private slots:
    void SearchGlobal();
    void IncrementProgress(int time);
    void MaxSteps();
    void setOptions();

signals:
    void PlotFinished(int runtype);
    void MultiScanFinished();
    void setValue(int value);
};
