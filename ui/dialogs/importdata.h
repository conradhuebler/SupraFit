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

#ifndef IMPORTDATA_H
#define IMPORTDATA_H

#include "core/data/dataclass.h"


#include <QtWidgets/QTableWidget>
#include <QtWidgets/QTableView>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QHeaderView>
#include <QtCore/QVector>
#include <QtCore/QPointer>

class QDialogButtonBox;
class QLineEdit;
class QPushButton;
class QSpinBox;

class SelectHeader : public QHeaderView
{
    
   Q_OBJECT 
   
public:  
    SelectHeader(QWidget *parent = 0) : QHeaderView(Qt::Horizontal, parent)
{
     connect(this, SIGNAL(sectionResized(int, int, int)), this, 
             SLOT(handleSectionResized(int)));
     connect(this, SIGNAL(sectionMoved(int, int, int)), this, 
             SLOT(handleSectionMoved(int, int, int)));
//       setMovable(true);
}
    ~SelectHeader(){ };
    void showEvent(QShowEvent *e)
    {
    for (int i=0;i<count();i++) {
       if (!boxes[i]) {
          QComboBox *box = new QComboBox(this);
          box->addItem("hier");
          box->addItem("dort");
          boxes[i] = box;
       }
       boxes[i]->setGeometry(sectionViewportPosition(i), 0, 
                                sectionSize(i) - 5, height());
       boxes[i]->show();
    }
    QHeaderView::showEvent(e);
    }
    void fixComboPositions()
    {   
    for (int i=0;i<count();i++)
        boxes[i]->setGeometry(sectionViewportPosition(i), 0, 
                                 sectionSize(i) - 5, height());
    }

private slots:
     void handleSectionResized(int i)
    {
    for (int j=visualIndex(i);j<count();j++) {
        int logical = logicalIndex(j);
        boxes[logical]->setGeometry(sectionViewportPosition(logical), 0, 
                                       sectionSize(logical) - 5, height());
    }
    }

    void handleSectionMoved(int logical, int oldVisualIndex, int newVisualIndex)
    {
    for (int i=qMin(oldVisualIndex, newVisualIndex);i<count();i++){
        int logical = logicalIndex(i);
        boxes[logical]->setGeometry(sectionViewportPosition(logical), 0, 
                                       sectionSize(logical) - 5, height());
    }
    }
    
private:
    QMap<int, QComboBox *> boxes;
};

class Table : public QTableView
{
    Q_OBJECT 
    
public:
    Table(QWidget *parent = 0) : QTableView(parent){
        header = new SelectHeader;
        setHorizontalHeader(header);
        
    }
    
    
private:
    void scrollContentsBy(int dx, int dy)
{
   QTableView::scrollContentsBy(dx, dy);
   if (dx != 0)
      header->fixComboPositions();
}

    SelectHeader *header;
    
    
};


class ImportData : public QDialog
{
    Q_OBJECT

public:
    ImportData(const QString &file, QWidget *parent = 0);
    ~ImportData();

     inline DataClass getStoredData(){ return *m_storeddata;}
private:
    
    void setUi();
    
    QPointer<QTableView > m_table;
    QPointer<QLineEdit > m_line;
    QPointer<QPushButton > m_file;
    QPointer<QSpinBox > m_conc, m_sign;
    QPointer<QDialogButtonBox > m_buttonbox;
    DataClass *m_storeddata;
    QString m_filename;
private slots:
    void LoadFile();
    void SelectFile();
    void accept();
    void NoChanged();
};

#endif // IMPORTDATA_H
