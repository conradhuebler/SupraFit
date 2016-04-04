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
#include <QtCore/QFile>
#include <QStandardItemModel>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTableView>

#include <QDebug>
#include "importdata.h"

ImportData::ImportData(QWidget *parent) :QDialog(parent)
{
    setUi();
    
}

ImportData::~ImportData()
{
}



void ImportData::setUi()
{
   QGridLayout *layout = new QGridLayout;
   
   
   m_buttonbox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
   
       connect(m_buttonbox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonbox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    
   m_line = new QLineEdit;
   m_file = new QPushButton("Load");
   connect(m_file, SIGNAL(clicked()), this, SLOT(LoadFile()));
   m_table = new Table;
   
   layout->addWidget(m_line, 0, 0);
   layout->addWidget(m_file, 0, 1);
   layout->addWidget(m_table, 1, 0, 1, 1);
   layout->addWidget(m_buttonbox, 3, 1);
   
   setLayout(layout);
}

void ImportData::LoadFile()
{
    QString filename = QFileDialog::getOpenFileName(this, "Select file", ".");
    
    
    
    
    QFile file(filename);
    if(!file.open(QIODevice::ReadOnly));
    {
        qDebug() << file.errorString();
//         return; //FIXME Hää
    }
    
    m_line->setText(filename);
    QStandardItemModel *model = new QStandardItemModel;
    QString f_content = file.readAll();
    QStringList lines = f_content.split("\n");
    foreach(const QString &line, lines)
    {
        QList<QStandardItem *> row;
        foreach(const QString &item, line.split("\t"))
            row.append(new QStandardItem(item));
        model->appendRow(row);
    }
    
    m_table->setModel(model);
        
}


#include "importdata.moc"
