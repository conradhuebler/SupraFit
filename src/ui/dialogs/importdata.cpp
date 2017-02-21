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

#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QKeyEvent>
#include <QtGui/QClipboard>
#include <QtCore/QFile>
#include <QStandardItemModel>
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
        QStringList lines = paste.split("\n");

        int cur_row = 0;
        QStandardItemModel *model = qobject_cast<QStandardItemModel *>(this->model());
        for(const QString line: qAsConst(lines))
        {
            int cur_col = 0;
            QStringList cells = line.simplified().split(" ");
            for(const QString &cell: qAsConst(cells))
            {
                model->setItem(cur_row, cur_col, new QStandardItem(QString(cell).replace(",", ".")));
                cur_col++;
            }
            cur_row++;
        }
    }
    else if (event->modifiers() & Qt::ControlModifier && event->key() == Qt::Key_C) 
    {
        
        /*QItemSelectionModel *select = this->selectionModel();

        if(!select->hasSelection())
            return; 
        QModelIndexList rows = select->selectedRows();
        QModelIndexList columns = select->selectedColumns();
        QString cliptext;
        
        foreach(const QModelIndex &column, columns)
        {    
            foreach(const QModelIndex &row, rows)
            {
                
            }
        }
        */
        /*
        QString paste =  QApplication::clipboard()->text();
        QStringList lines = paste.split("\n");
        QModelIndex index = currentIndex();
        int row = index.row();
        int column = index.column();
        QStandardItemModel *model = qobject_cast<QStandardItemModel *>(this->model());
        foreach(const QString line, lines)
        {
            int col = column;
            QStringList cells = line.simplified().split(" ");
            foreach(const QString &cell, cells)
            {
                model->item(row, col)->setData(QString(cell).replace(",", "."), Qt::DisplayRole);
                col++;
            }
            row++;
        }*/
    }
    else {
        
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
    
    QStandardItemModel *model = new QStandardItemModel(0,0);
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
    m_conc->setMinimum(2);
    m_sign = new QSpinBox;
    m_sign->setMinimum(1);
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
    layout->addWidget(m_file, 0, 2);
    layout->addWidget(m_export, 0, 3);
    layout->addWidget(m_switch_concentration, 0, 4);
    layout->addWidget(new QLabel(tr("No. Conc:")), 1, 0);
    layout->addWidget(m_conc, 1, 1);
    layout->addWidget(new QLabel(tr("No. Signals:")), 1, 2);
    layout->addWidget(m_sign, 1, 3);
    layout->addWidget(m_table, 3, 0, 1, 4);
    layout->addWidget(m_buttonbox, 4, 1, 1, 4);
    
    setLayout(layout);
}

void ImportData::NoChanged()
{
    m_conc->setMaximum(2); //FIXME for now
    m_sign->setMaximum(m_table->model()->columnCount()  - 2);
}


void ImportData::LoadFile()
{ 
    m_line->setText(m_filename);
    FileHandler *filehandler = new FileHandler(m_filename, this); 
    
    if(filehandler->FileSupported())
    {
        QStandardItemModel *model = filehandler->getData(); 
        m_table->setModel(model);
    }else
        QMessageBox::warning(this, QString("File not supported!"), QString("Sorry, but I don't know this format. Try a simple table."));
    
    delete filehandler;
    NoChanged();
}

void ImportData::SelectFile()
{
    m_filename = QFileDialog::getOpenFileName(this, "Select file", ".");
    LoadFile();
}

void ImportData::ExportFile()
{
    QString filename = QFileDialog::getSaveFileName(this, "Select file", ".");
    if(filename.isEmpty())
        return;
    QStandardItemModel *model = qobject_cast<QStandardItemModel *>(m_table->model());
    
    int rows = model->rowCount() - 1; 
    int columns = model->columnCount(model->indexFromItem(model->invisibleRootItem()));
    
    QFile file(filename);
    if(!file.open(QIODevice::ReadWrite))
        return;
    QTextStream stream(&file);
    for(int i = 0; i < rows; ++i)
    {
        QVector<qreal > conc, sign;
        for(int j = 0; j < columns; ++j)
        {
            if(!model->item(i, j)->data(Qt::DisplayRole).toString().isNull() && !model->item(i, j)->data(Qt::DisplayRole).toString().isEmpty())
            {
                if(j < m_conc->value())
                    conc << (model->item(i, j)->data(Qt::DisplayRole).toDouble());
                else 
                    sign << (model->item(i, j)->data(Qt::DisplayRole).toDouble());
            }
        }
        if(m_switch_concentration->isChecked())
        {
            qreal a = conc[1];
            conc[1] = conc[0];
            conc[0] = a;
        }
        for(double d: conc)
            stream << d << " ";
        for(double d: sign)
            stream << d << " ";
        stream <<  endl;
    }
    
}

void ImportData::accept()
{
    m_storeddata = new DataClass(DataClass::DiscretData); //TODO for spectra this must be changeable
    
    QStandardItemModel *model = qobject_cast<QStandardItemModel *>(m_table->model());
    QStringList header;
    int rows = model->rowCount() - 1; 
    int columns = model->columnCount(model->indexFromItem(model->invisibleRootItem()));
    for(int i = 0; i < rows; ++i)
    {
        bool import = true;
        QVector<qreal > conc, sign;
        for(int j = 0; j < columns; ++j)
        {
            if(!model->item(i, j)->data(Qt::DisplayRole).toString().isNull() && !model->item(i, j)->data(Qt::DisplayRole).toString().isEmpty())
            {
                bool ok;
                qreal var = model->item(i, j)->data(Qt::DisplayRole).toDouble(&ok);
                
                if(ok)
                {                
                    if(j < m_conc->value())
                        conc << var;
                    else 
                        sign << var;
                }else
                {
                    header << model->item(i, j)->data(Qt::DisplayRole).toString();
                    import = false;
                }
            }
            else
            {
                import = false;
                break;
            }
        }
        if(m_switch_concentration->isChecked())
        {
            qreal a = conc[1];
            conc[1] = conc[0];
            conc[0] = a;
        }
        if(import)
            m_storeddata->addPoint(conc, sign);
    }
    m_storeddata->setHeader(header);
    QDialog::accept();
}


#include "importdata.moc"
