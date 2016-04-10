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

#include "core/data/dataclass.h"
#include "core/data/modelclass.h"

#include "ui/dialogs/importdata.h"
#include "ui/widgets/modelwidget.h"
#include "ui/widgets/datawidget.h"
#include "ui/widgets/chartwidget.h"
#include "ui/widgets/modeldataholder.h"

#include <QtWidgets/QLabel>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QAction>
#include <QtWidgets/QSplitter>

#include <QDebug>

#include "nmr2fit.h"

MainWindow::MainWindow()
{
    m_mainsplitter = new QSplitter(Qt::Horizontal);
    
    
    
    setCentralWidget(m_mainsplitter);
    
    
    m_model_dataholder = new ModelDataHolder;
    m_charts = new ChartWidget;
    m_model_dataholder->setChartWidget(m_charts);
    m_mainsplitter->addWidget(m_model_dataholder);
    m_mainsplitter->addWidget(m_charts);
    
    QAction *loadaction = new QAction(this);
    loadaction->setText("Import Data");
    connect(loadaction, SIGNAL(triggered(bool)), this, SLOT(LoadData()));
    
    QAction* quitaction= new QAction(this);
    quitaction->setText( "Quit" );
    connect(quitaction, SIGNAL(triggered()), SLOT(close()) );
    
    QMenu *filemenu =  menuBar()->addMenu( "File" );
    filemenu->addAction( loadaction );
    filemenu->addAction( quitaction );
}

MainWindow::~MainWindow()
{
    
    
}


void MainWindow::LoadData()
{
    ImportData dialog(this);
    
    dialog.exec();
    
    DataClass storeddata = dialog.getStoredData();
    m_model_dataholder->setData(storeddata);

}



void MainWindow::resizeEvent(QResizeEvent* event)
{
    m_charts->resize(3*event->size().width()/4, m_charts->height());
    m_model_dataholder->resize(event->size().width()/4, m_model_dataholder->height());

    QWidget::resizeEvent(event);
}

#include "nmr2fit.moc"
