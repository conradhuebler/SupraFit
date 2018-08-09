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

#include "src/ui/guitools/chartwrapper.h"

#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QValueAxis>

#include <QtCore/QPointer>
#include <QtCore/QVector>

#include <QtWidgets/QWidget>

#include "src/ui/widgets/chartview.h"

class AbstractModel;

class QAction;
class QComboBox;
class QMenu;
class QPushButton;
class QChartView;
class ChartWrapper;

struct Charts {
    QSharedPointer<ChartWrapper> error_wrapper;
    QSharedPointer<ChartWrapper> signal_wrapper;
    QSharedPointer<ChartWrapper> data_wrapper;
};

class ChartDockTitleBar : public QWidget {
    Q_OBJECT
public:
    ChartDockTitleBar();

protected:
    inline void mouseDoubleClickEvent(QMouseEvent* event) override { event->ignore(); }
    inline void mousePressEvent(QMouseEvent* event) override { event->ignore(); }
    inline void mouseReleaseEvent(QMouseEvent* event) override { event->ignore(); }
    inline void mouseMoveEvent(QMouseEvent* event) override { event->ignore(); }

signals:
    void close();
    void ThemeChanged(QtCharts::QChart::ChartTheme theme);
    void AnimationChanged(bool animation);
    void ChartFlip(bool flip);
    void setSize(int size);

private:
    QPushButton *m_hide, *m_tools;
    QMenu *m_theme, *m_size;
    QAction *m_flip, *m_animation;

private slots:
    void ThemeChange(QAction* action);
};

class ChartWidget : public QWidget {
    Q_OBJECT

public:
    ChartWidget();
    ~ChartWidget();
    QSharedPointer<ChartWrapper> setRawData(QSharedPointer<DataClass> rawdata);
    Charts addModel(QSharedPointer<AbstractModel> model);
    inline ChartDockTitleBar* TitleBarWidget() const { return m_TitleBarWidget; }

private:
    qreal max_shift, min_shift;

    QPointer<ChartView> m_signalview, m_errorview;
    QPointer<QtCharts::QChart> m_signalchart, m_errorchart;
    QPointer<QtCharts::QValueAxis> m_x_chart, m_y_chart, m_x_error, m_y_error;
    QVector<QWeakPointer<AbstractModel>> m_models;
    QWeakPointer<DataClass> m_rawdata;
    ChartDockTitleBar* m_TitleBarWidget;
    QVector<QVector<int>> m_titration_curve, m_model_curve, m_error_curve;
    QSharedPointer<ChartWrapper> m_data_mapper;
    QString m_signal_x, m_signal_y, m_error_x, m_error_y;

private slots:
    void formatAxis();
    void Repaint();
    void updateUI();
    void updateTheme(QtCharts::QChart::ChartTheme theme);
    void setAnimation(bool animation);
    void stopAnimiation();
    void restartAnimation();
};
