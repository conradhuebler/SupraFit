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
#include "src/core/dataclass.h"
#include "src/core/filehandler.h"
#include "src/global.h"

#include <QtCore/QFile>

#include <QtGui/QKeyEvent>
#include <QtGui/QClipboard>

#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QLabel>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTableView>
#include <QtWidgets/QMessageBox>

#include <QDebug>
#include "importdata.h"

void TableView::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_C && event->modifiers() & Qt::ControlModifier) 
    {
        QApplication::clipboard()->setText( this->currentIndex().data().toString() );
    }
    else if (event->modifiers() & Qt::ControlModifier && event->key() == Qt::Key_V) 
    {
        QString paste =  QApplication::clipboard()->text();
        FileHandler *handler = new FileHandler(this);
        handler->setFileContent(paste);
        DataTable *model = handler->getData();
        setModel(model);
        delete handler;
    } else {
        
        QTableView::keyPressEvent(event);
    }
}


ImportData::ImportData(const QString &file, QWidget *parent) : QDialog(parent), m_filename(file)
{
    setUi();
    LoadFile();
}

ImportData::ImportData(QWidget *parent) : QDialog(parent)
{
    setUi(); 
    DataTable *model = new DataTable(0,0, this);
    m_table->setModel(model);
}


ImportData::~ImportData()
{
    if(m_storeddata)
        delete m_storeddata;
}



void ImportData::setUi()
{
    QGridLayout *layout = new QGridLayout;
    
    m_buttonbox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    m_switch_concentration = new QCheckBox;
    m_switch_concentration->setText("Switch Host/Guest");
    connect(m_buttonbox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonbox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    
    m_conc = new QSpinBox;
    connect(m_conc, SIGNAL(editingFinished()), this, SLOT(NoChanged()));
    m_sign = new QSpinBox;
    connect(m_sign, SIGNAL(editingFinished()), this, SLOT(NoChanged()));
    m_line = new QLineEdit;
    m_select = new QPushButton("Select file");
    connect(m_select, SIGNAL(clicked()), this, SLOT(SelectFile()));
    m_file = new QPushButton("Load");
    m_export = new QPushButton("Export table");
    connect(m_export, SIGNAL(clicked()), this, SLOT(ExportFile()));
    connect(m_file, SIGNAL(clicked()), this, SLOT(LoadFile()));
    m_table = new TableView;
    
    layout->addWidget(m_select, 0, 0);
    layout->addWidget(m_line, 0, 1);
//     layout->addWidget(m_file, 0, 2);
    layout->addWidget(m_export, 0, 3);
//     layout->addWidget(m_switch_concentration, 0, 4);
    layout->addWidget(new QLabel(tr("No. of indepdent variables:")), 1, 0);
    layout->addWidget(m_conc, 1, 1);
//     layout->addWidget(new QLabel(tr("No. Signals:")), 1, 2);
//     layout->addWidget(m_sign, 1, 3);
    layout->addWidget(m_table, 3, 0, 1, 4);
    layout->addWidget(m_buttonbox, 4, 1, 1, 4);
    
    setLayout(layout);
    setWindowTitle(tr("Import Table"));
    resize(800,600);
}

void ImportData::NoChanged()
{
    m_conc->setMinimum(1);
    m_conc->setMaximum(m_table->model()->columnCount()  - 1);
    m_sign->setMaximum(m_table->model()->columnCount()  - 2);
    if(m_table->model()->columnCount()  > 2)
        m_conc->setValue(2);
}


void ImportData::LoadFile()
{ 
    m_line->setText(m_filename);
    FileHandler *filehandler = new FileHandler(m_filename, this); 
    
    if(filehandler->FileSupported())
    {
        DataTable *model = filehandler->getData(); 
        m_table->setModel(model);
    }else
        QMessageBox::warning(this, QString("File not supported!"), QString("Sorry, but I don't know this format. Try a simple table."));
    
    delete filehandler;
    NoChanged();
}

void ImportData::SelectFile()
{
    m_filename = QFileDialog::getOpenFileName(this, "Select file", getDir());
    setLastDir(m_filename);
    LoadFile();
}

void ImportData::ExportFile()
{
    QString filename = QFileDialog::getSaveFileName(this, "Select file", getDir());
    if(filename.isEmpty())
        return;
    
    setLastDir(filename);
    DataTable *model = qobject_cast<DataTable *>(m_table->model());
    
    QFile file(filename);
    if(!file.open(QIODevice::ReadWrite))
        return;
    
    QTextStream stream(&file);
    stream << model->ExportAsString();
}

void ImportData::WriteData(const DataTable* model, int independent)
{
    independent = m_conc->value();
    m_storeddata = new DataClass(DataClass::DiscretData); //TODO for spectra this must be changeable
    DataTable *concentration_block = model->BlockColumns(0,independent);
    DataTable *signals_block = model->BlockColumns(independent,model->columnCount() -independent );
    m_storeddata->setSignalTable( signals_block );
    m_storeddata->setConcentrationTable( concentration_block );
}


void ImportData::accept()
{
    DataTable *model = qobject_cast<DataTable *>(m_table->model());
    WriteData(model);
    QDialog::accept();
}


#include "importdata.moc"
