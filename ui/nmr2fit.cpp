
#include "nmr2fit.h"


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


MainWindow::MainWindow()
{
    m_mainsplitter = new QSplitter(Qt::Horizontal);
    
    
    
    setCentralWidget(m_mainsplitter);
    
    
    m_model_dataholder = new ModelDataHolder;
    m_charts = new ChartWidget;
    
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
{}


void MainWindow::LoadData()
{
    ImportData dialog(this);
    
    dialog.exec();
}

#include "nmr2fit.moc"
