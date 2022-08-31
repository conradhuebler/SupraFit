/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016 - 2022 Conrad Hübler <Conrad.Huebler@gmx.net>
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
#include "src/core/models/dataclass.h"

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
class QScrollArea;
class QDoubleSpinBox;
class SignalElement;
class SystemParameterWidget;
class RegressionAnalysisDialog;
class QTextEdit;

class DataWidget : public QWidget {
    Q_OBJECT

public:
    DataWidget();
    ~DataWidget();
    void setData(QWeakPointer<DataClass> dataclass, QWeakPointer<ChartWrapper> wrapper);
    void setEditable(bool editable);
    virtual QSize minimumSizeHint() const override { return QSize(800, 600); }

private:
    void UpdateRanges();

    QCheckBox* m_plot_x;
    QTableView *m_concentrations, *m_signals;
    QTextEdit* m_text_edit;
    QPushButton *m_switch, *m_linear, *m_hide_points;
    QWeakPointer<DataClass> m_data;
    QWeakPointer<ChartWrapper> m_wrapper;
    QLineEdit *m_name, *m_x_model, *m_y_model;
    QCheckBox *m_x_raw, *m_y_raw;
    QLabel *m_x_string, *m_y_string, *m_range;
    QLabel *m_datapoints, *m_substances, *m_const_subs, *m_signals_count;
    QWidget *m_tables, *m_widget;
    QSplitter* m_splitter;
    QVector<QPointer<SignalElement>> m_signal_elements;
    QGridLayout *m_layout, *m_tables_layout;
    QList<QPointer<QDoubleSpinBox>> m_scaling_boxes;
    QScrollArea* m_series_scroll_area;
    QWidget* m_systemwidget;
    bool m_system_parameter_loaded;
    RegressionAnalysisDialog* dialog;

    int m_index_x = -1, m_index_y = -1;

private slots:
    void ShowContextMenu(const QPoint& pos);
    void SetProjectName();
    void HidePoint();
    void LinearAnalysis();

signals:
    void recalculate();
    void NameChanged();
};
