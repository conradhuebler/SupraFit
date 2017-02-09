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

#include "src/ui/dialogs/chartconfig.h"


#include <QtWidgets/QMenu>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLayout>
#include <QDrag>
#include <QtCharts/QChart>
#include <QtCharts/QLegendMarker>
#include <QtCharts/QValueAxis>
#include <QtCharts/QXYSeries>
#include <QDebug>
#include <QtCore/QBuffer>
#include <QtCore/QMimeData>

#include <cmath>
#include <iostream>
#include "chartview.h"

void ChartViewPrivate::mousePressEvent(QMouseEvent *event)
{
    QImage image(scene()->sceneRect().size().toSize(), QImage::Format_ARGB32);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    scene()->render(&painter);
    QPixmap pixmap = QPixmap::fromImage(image);
    QByteArray itemData;
    QBuffer outputBuffer(&itemData);
        
    outputBuffer.open(QIODevice::WriteOnly);
    pixmap.toImage().save(&outputBuffer, "PNG");

    QMimeData *mimeData = new QMimeData;
    mimeData->setData("image/png", itemData);

    QDrag *drag = new QDrag(this);
    drag->setMimeData(mimeData);
    drag->setPixmap(pixmap);
    drag->setHotSpot(event->pos());
    
    drag->exec(Qt::CopyAction);
}

void ChartViewPrivate::dragEnterEvent(QDragEnterEvent *event)
{
    
     if (event->mimeData()->hasFormat("image/png")) {
        if (event->source() == this) {
            event->setDropAction(Qt::CopyAction);
            event->accept();
        } else {
            event->acceptProposedAction();
        }
    } else {
        event->ignore();
    }   
}

void ChartViewPrivate::dragMoveEvent(QDragMoveEvent *event)
{
    
      if (event->mimeData()->hasFormat("image/png")) {
        if (event->source() == this) {
            event->setDropAction(Qt::CopyAction);
            event->accept();
        } else {
            event->acceptProposedAction();
        }
    } else {
        event->ignore();
    }    
}

void ChartViewPrivate::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Plus:
        chart()->zoomIn();
        break;
    case Qt::Key_Minus:
        chart()->zoomOut();
        break;
    case Qt::Key_Left:
        chart()->scroll(-10, 0);
        break;
    case Qt::Key_Right:
        chart()->scroll(10, 0);
        break;
    case Qt::Key_Up:
        chart()->scroll(0, 10);
        break;
    case Qt::Key_Down:
        chart()->scroll(0, -10);
        break;
    default:
        QGraphicsView::keyPressEvent(event);
        break;
    }
}

ChartView::ChartView(QtCharts::QChart *chart) : m_chart_private(new ChartViewPrivate(chart, this)), m_chart(chart), has_legend(false), connected(false), m_x_axis(QString()), m_y_axis(QString()), m_pending(false)
{
    setUi();
    m_chart->legend()->setAlignment(Qt::AlignRight);
    
}

ChartView::ChartView() : has_legend(false), connected(false), m_x_axis(QString()), m_y_axis(QString()), m_pending(false)
{
    m_chart = new QtCharts::QChart(); 
    m_chart_private = new ChartViewPrivate(new ChartViewPrivate(m_chart, this));
    setUi();
    m_chart->legend()->setAlignment(Qt::AlignRight);
    
}

void ChartView::setUi()
{
    QGridLayout *layout = new QGridLayout;
    QMenu *menu = new QMenu;
    
    QAction *plotsettings = new QAction(this);
    plotsettings->setText(tr("Plot Settings"));
        connect(plotsettings, SIGNAL(triggered()), this, SLOT(PlotSettings()));
        menu->addAction(plotsettings);
    
    QAction *scaleAction = new QAction(this);
    scaleAction->setText(tr("Rescale Axis"));
        connect(scaleAction, SIGNAL(triggered()), this, SLOT(formatAxis()));
        menu->addAction(scaleAction);    
        
    QAction *printplot = new QAction(this);
    printplot->setText(tr("Print Diagram"));
        connect(printplot, SIGNAL(triggered()), this, SLOT(PlotSettings()));
        menu->addAction(printplot);
       
    QAction *exportlatex = new QAction(this);
    exportlatex->setText(tr("Export to Latex (tikz)"));
        connect(exportlatex, SIGNAL(triggered()), this, SLOT(ExportLatex()));
        menu->addAction(exportlatex);
    
    QAction *exportgnuplot = new QAction(this);
    exportgnuplot->setText(tr("Export to Latex (tikz)"));
        connect(exportgnuplot, SIGNAL(triggered()), this, SLOT(ExportGnuplot()));
        menu->addAction(exportgnuplot);
    
    
    m_config = new QPushButton(tr("Tools"));
        m_config->setFlat(true);
        m_config->setIcon(QIcon::fromTheme("applications-system"));
        m_config->setMaximumWidth(100);
        m_config->setStyleSheet("QPushButton {background-color: #A3C1DA; color: black;}");
    m_config->setMenu(menu);
    
    layout->addWidget(m_chart_private, 0, 0, 1, 5);//, Qt::AlignHCenter);
    layout->addWidget(m_config, 0, 4, Qt::AlignTop);
    setLayout(layout);
    
    connect(&m_chartconfigdialog, SIGNAL(ConfigChanged(ChartConfig)), this, SLOT(setChartConfig(ChartConfig)));
    connect(&m_chartconfigdialog, SIGNAL(ScaleAxis()), this, SLOT(formatAxis()));
}


void ChartView::addSeries(  QtCharts::QAbstractSeries* series , bool legend)
{    
    if(!m_chart->series().contains(series))
    {
        m_chart->addSeries(series);
        m_chart->createDefaultAxes();
    }
    m_chart->legend()->markers(series).first()->setVisible(legend);
    if(!connected)
        if(connect(this, SIGNAL(AxisChanged()), this, SLOT(formatAxis())))
            connected = true;
}


void ChartView::formatAxis()
{
    if(m_pending)
        return;
    m_pending = true;
    qreal x_min = 0;
    qreal x_max = 0;
    qreal y_max = 0;
    qreal y_min = 0;
    for(QtCharts::QAbstractSeries *series: m_chart->series())
    {
        QtCharts::QXYSeries *serie = qobject_cast<QtCharts::QXYSeries *>(series);
        if(serie->isVisible())
        {
            QVector<QPointF> points = serie->pointsVector();
            for(int i = 0; i < points.size(); ++i)
            {
                y_min = qMin(y_min, points[i].y());
                y_max = qMax(y_max, points[i].y());
                
                x_min = qMin(x_min, points[i].x());
                x_max = qMax(x_max, points[i].x());
            }
        }
    }
    QtCharts::QValueAxis *y_axis = qobject_cast<QtCharts::QValueAxis *>( m_chart->axisY());
    y_axis->setMax(y_max);
    y_axis->setMin(y_min);
    y_axis->applyNiceNumbers();
    y_axis->setTitleText(m_y_axis);

    
     QtCharts::QValueAxis *x_axis = qobject_cast<QtCharts::QValueAxis *>( m_chart->axisX());
     x_axis->setMax(x_max*1.1);
     x_axis->setMin(x_min);
     /*
      * In some cases when x_max is slightly higher than five, the nice numbers result in x_max = 10, which looks not nice at all
      */
     if(fmod(x_max,5) > 1)
        x_axis->applyNiceNumbers();
     x_axis->setTitleText(m_x_axis);
     m_pending = false;
}


void ChartView::MaxValueChanged(qreal value)
{
    qDebug() << value;
}

void ChartView::PlotSettings()
{
    if(!connected)
        return;
    m_chartconfigdialog.setConfig(getChartConfig());
    m_chartconfigdialog.show();
}

void ChartView::setChartConfig(const ChartConfig& chartconfig)
{
    QtCharts::QValueAxis *x_axis = qobject_cast<QtCharts::QValueAxis *>( m_chart->axisX());
    
    x_axis->setTitleText(chartconfig.x_axis);
    x_axis->setTickCount(chartconfig.x_step);
    x_axis->setMin(chartconfig.x_min);
    x_axis->setMax(chartconfig.x_max);
    QtCharts::QValueAxis *y_axis = qobject_cast<QtCharts::QValueAxis *>( m_chart->axisY()); 
    y_axis->setTitleText(chartconfig.y_axis);
    y_axis->setTickCount(chartconfig.y_step);
    y_axis->setMin(chartconfig.y_min);
    y_axis->setMax(chartconfig.y_max);
}

ChartConfig ChartView::getChartConfig() const
{
    QtCharts::QValueAxis *x_axis = qobject_cast<QtCharts::QValueAxis *>( m_chart->axisX());
    QtCharts::QValueAxis *y_axis = qobject_cast<QtCharts::QValueAxis *>( m_chart->axisY());
    
    ChartConfig chartconfig;
    chartconfig.x_axis = x_axis->titleText();
    chartconfig.x_min = x_axis->min();
    chartconfig.x_max = x_axis->max();
    chartconfig.x_step = x_axis->tickCount();
    chartconfig.y_axis = y_axis->titleText();
    chartconfig.y_min = y_axis->min();
    chartconfig.y_max = y_axis->max();
    chartconfig.y_step = y_axis->tickCount();
    
    return chartconfig;
}


void ChartView::PrintPlot()
{
    
}

void ChartView::ExportLatex()
{
}

void ChartView::ExportGnuplot()
{
}

#include "chartview.moc"
