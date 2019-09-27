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

#ifndef SIGNALELEMENT_H
#define SIGNALELEMENT_H

#include <QtCore/QPointer>
#include <QtWidgets/QGroupBox>

class QLineEdit;
class QCheckBox;
class QPushButton;
class QDoubleSpinBox;

class ScatterSeries;
class ChartWrapper;

class SignalElement : public QGroupBox {
    Q_OBJECT

public:
    SignalElement(QWeakPointer<DataClass> data, QWeakPointer<ChartWrapper> wrapper, int index, QWidget* parent = 0);
    ~SignalElement();

public slots:
    void HideSeries();

private:
    QWeakPointer<DataClass> m_data;
    QWeakPointer<ChartWrapper> m_wrapper;
    QLineEdit* m_name;
    QCheckBox *m_show, *m_rectangle;
    QPushButton *m_choose, *m_toggle;
    QPointer<ScatterSeries> m_data_series;
    QDoubleSpinBox* m_markerSize;
    QColor m_color;
    int m_index;
    bool m_series_hidden = false;

private slots:
    void ToggleSeries(int i);
    void chooseColor();
    void ColorChanged(const QColor& color);
    void ShowLine(int i);
    void setName(const QString& str);
    void setMarkerSize(qreal value);
    void setMarkerShape(int shape);
    void togglePlot();
    void UnCheckToggle(int i);
};

#endif
