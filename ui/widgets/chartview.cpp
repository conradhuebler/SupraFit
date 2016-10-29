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
#include <QtWidgets/QMenu>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLayout>
#include <QDrag>
#include <QtCharts/QChart>
#include <QtCore/QBuffer>
#include <QtCore/QMimeData>
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

ChartView::ChartView(QtCharts::QChart *chart) : m_chart_private(new ChartViewPrivate(chart, this))
{
    setUi();
    
}

void ChartView::setUi()
{
    QGridLayout *layout = new QGridLayout;
    QMenu *menu = new QMenu;
    QAction *plotsettings = new QAction(this);
    plotsettings->setText(tr("Plot Settings"));
        connect(plotsettings, SIGNAL(triggered()), this, SLOT(PlotSettings()));
        menu->addAction(plotsettings);
    
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
}

void ChartView::PlotSettings()
{
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
