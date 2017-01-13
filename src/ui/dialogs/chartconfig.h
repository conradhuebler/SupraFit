/*
 * <one line to give the program's name and a brief idea of what it does.>
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

#ifndef CHARTCONFIG_H
#define CHARTCONFIG_H
#include <QtWidgets/QDialog>

class QLineEdit;
class QLabel;
class QDoubleSpinBox;
class QSpinBox;
class QDialogButtonBox;

struct ChartConfig
{
    QString x_axis = QString();
    QString y_axis = QString();
    qreal x_min = 0;
    qreal x_max = 5;
    int x_step = 5;
    qreal y_min = 0;
    qreal y_max = 7;
    int y_step = 5;
};

class ChartConfigDialog : public QDialog
{
    Q_OBJECT
public:
    ChartConfigDialog();
    ~ChartConfigDialog();
    void setConfig(const ChartConfig &chartconfig);
    ChartConfig Config() const { return m_chartconfig; }
    QDialogButtonBox *m_buttons;
private:
    QPushButton *m_scaleaxis;
    QLineEdit *m_x_axis, *m_y_axis;
    QDoubleSpinBox *m_x_min, *m_x_max,  *m_y_min, *m_y_max;
    QSpinBox *m_x_step,*m_y_step;
    ChartConfig m_chartconfig;
private slots:
    void Changed();
signals:
    void ConfigChanged(ChartConfig chartconfig);
    void ScaleAxis();
};

#endif // CHARTCONFIG_H
