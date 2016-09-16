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
#include "core/filehandler.h"


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

ImportData::ImportData(const QString &file, QWidget *parent) : QDialog(parent), m_filename(file)
{
    setUi();
    LoadFile();
    
}

ImportData::~ImportData()
{
    delete m_storeddata;
}



void ImportData::setUi()
{
   QGridLayout *layout = new QGridLayout;
   
   
   m_buttonbox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
   
       connect(m_buttonbox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonbox, &QDialogButtonBox::rejected, this, &QDialog::reject);
   
    
    m_conc = new QSpinBox;
    connect(m_conc, SIGNAL(editingFinished()), this, SLOT(NoChanged()));
    m_conc->setMinimum(2);
    m_sign = new QSpinBox;
    m_sign->setMinimum(1);
    connect(m_sign, SIGNAL(editingFinished()), this, SLOT(NoChanged()));
   m_line = new QLineEdit;
   m_file = new QPushButton("Load");
   connect(m_file, SIGNAL(clicked()), this, SLOT(LoadFile()));
   m_table = new QTableView;
   
   layout->addWidget(m_line, 0, 0);
   layout->addWidget(m_file, 0, 1);
   layout->addWidget(new QLabel(tr("No. Conc:")), 1, 0);
   layout->addWidget(m_conc, 1, 1);
   layout->addWidget(new QLabel(tr("No. Signals:")), 1, 2);
   layout->addWidget(m_sign, 1, 3);
   layout->addWidget(m_table, 3, 0, 1, 2);
   layout->addWidget(m_buttonbox, 4, 1);
   
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


void ImportData::accept()
{
    m_storeddata = new DataClass(DataClass::DiscretData); //TODO for spectra this must be changeable
    
    QStandardItemModel *model = qobject_cast<QStandardItemModel *>(m_table->model());
    
    int rows = model->rowCount() - 1; 
    int columns = model->columnCount(model->indexFromItem(model->invisibleRootItem()));
    for(int i = 0; i < rows; ++i)
    {
            QVector<qreal > conc, sign;
            for(int j = 0; j < columns; ++j)
            {
                if(j < m_conc->value())
                    conc << (model->item(i, j)->data(Qt::DisplayRole).toDouble());
                else 
                    sign << (model->item(i, j)->data(Qt::DisplayRole).toDouble());
            }
        m_storeddata->addPoint(conc, sign);
    }
    QDialog::accept();
}


#include "importdata.moc"
