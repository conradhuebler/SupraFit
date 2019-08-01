/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016 - 2019 Conrad Hübler <Conrad.Huebler@gmx.net>
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
#include "src/core/dataclass.h"

#include <QtCore/QPointer>
#include <QtCore/QVector>

#include <QtWidgets/QGroupBox>
#include <QtWidgets/QTableView>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QWidget>

class QPushButton;
class QLabel;
class QGroupBox;
class ChartWrapper;
class QLineEdit;
class QCheckBox;
class QGridLayout;
class QSplitter;
class ScatterSeries;
class QDoubleSpinBox;
class SignalElement;
class SystemParameterWidget;
class RegressionAnalysisDialog;

class DataWidget : public QWidget {
    Q_OBJECT

public:
    DataWidget();
    ~DataWidget();
    void setData(QWeakPointer<DataClass> dataclass, QWeakPointer<ChartWrapper> wrapper);
    void setEditable(bool editable);

private:
    QTableView *m_concentrations, *m_signals;
    QPushButton *m_switch, *m_linear, *m_hide_points;
    QWeakPointer<DataClass> m_data;
    QWeakPointer<ChartWrapper> m_wrapper;
    QLineEdit* m_name;
    QLabel *m_datapoints, *m_substances, *m_const_subs, *m_signals_count;
    QWidget *m_tables, *m_widget;
    QSplitter* m_splitter;
    QVector<QPointer<SignalElement>> m_signal_elements;
    QGridLayout* layout;
    QList<QPointer<QDoubleSpinBox>> m_scaling_boxes;
    QWidget* m_systemwidget;
    bool m_system_parameter_loaded;
    RegressionAnalysisDialog* dialog;

private slots:
    void ShowContextMenu(const QPoint& pos);
    void switchHG();
    void SetProjectName();
    void setScaling();
    void HidePoint();
    void LinearAnalysis();

signals:
    void recalculate();
    void NameChanged();
};
