/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016 - 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/ui/mainwindow/chartwidget.h"

class SpinBox;
class HoverCheckBox;
class QLineEdit;

class ModelElement : public QGroupBox {
    Q_OBJECT
public:
    ModelElement(QSharedPointer<AbstractModel> model, Charts charts, int no, QWidget* parent = 0);
    ~ModelElement();
    QVector<double> D() const;
    bool Include() const;

public slots:
    void Update();
    void ToggleSeries(int);
    void ChangeColor(const QColor& color);
    void setReadOnly(bool readonly);
    void setLabel(const QString& str);

private:
    QVector<SpinBox*> m_constants;
    QLabel* m_error;
    QVector<QLabel*> m_labels;
    QPushButton *m_remove, *m_optimize, *m_plot, *m_toggle;
    QCheckBox *m_include, *m_legend;
    HoverCheckBox* m_show;
    QWeakPointer<AbstractModel> m_model;
    QPointer<LineSeries> m_error_series, m_signal_series;
    QPointer<QLineEdit> m_name;

    int m_no;
    QColor m_color;
    Charts m_charts;
    void DisableSignal(int state);

protected:
#pragma message("Restore or clean up before release")
    /*
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    */
private slots:
    void chooseColor();
    void togglePlot();
    void toggleActive();
    void UnCheckToggle(int i);

signals:
    void ValueChanged();
    void Minimize(int i);
    void SetColor();
    void ActiveSignalChanged();
    void ColorChanged(const QColor& color);
    void LocalCheckState(int state);
};
