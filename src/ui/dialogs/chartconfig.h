/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016 - 2018  Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>

class QCheckBox;
class QComboBox;
class QLineEdit;
class QLabel;
class QDoubleSpinBox;
class QSpinBox;
class QDialogButtonBox;

struct ChartConfig {
    QString x_axis = QString();
    QString y_axis = QString();
    QString title = QString();
    qreal x_min = 0;
    qreal x_max = 5;
    int x_step = 5;
    qreal y_min = 0;
    qreal y_max = 7;
    int x_size = qApp->instance()->property("xSize").toInt();
    int y_size = qApp->instance()->property("ySize").toInt();
    int scaling = qApp->instance()->property("chartScaling").toInt();
    double markerSize = qApp->instance()->property("markerSize").toDouble();
    double lineWidth = qApp->instance()->property("lineWidth").toDouble();

    int y_step = 5;
    int Theme = 0;
    bool m_legend = false, m_lock_scaling = false, m_annotation = true, showAxis = true;
    QFont m_label, m_ticks, m_keys, m_title;
    Qt::Alignment align;
};

class ChartConfigDialog : public QDialog {
    Q_OBJECT
public:
    ChartConfigDialog(QWidget* widget);
    ~ChartConfigDialog();
    void setConfig(const ChartConfig& chartconfig);
    inline ChartConfig Config() const { return m_chartconfig; }
    QDialogButtonBox* m_buttons;

private:
    QPushButton *m_scaleaxis, *m_keys, *m_labels, *m_ticks, *m_alignment, *m_titlefont, *m_resetFontConfig;
    QLineEdit *m_x_axis, *m_y_axis, *m_title;
    QDoubleSpinBox *m_x_min, *m_x_max, *m_y_min, *m_y_max, *m_markerSize, *m_lineWidth;
    QSpinBox *m_x_step, *m_y_step, *m_scaling, *m_x_size, *m_y_size;
    ChartConfig m_chartconfig;
    QCheckBox *m_legend, *m_lock_scaling, *m_annotation, *m_show_axis;
    QComboBox* m_theme;

private slots:
    void Changed();
    void setTicksFont();
    void setKeysFont();
    void setLabelFont();
    void setTitleFont();

signals:
    void ConfigChanged(ChartConfig chartconfig);
    void ScaleAxis();
    void ResetFontConfig();
};
