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

#include "src/core/dataclass.h"


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
class QCheckBox;
class QStandardItemModel;

class DataTable;

class TableView : public QTableView
{
    Q_OBJECT

public:
    inline TableView() : QTableView() {}
    inline ~TableView() { }
    
protected:
  virtual void keyPressEvent(QKeyEvent *event);
    
};



class ImportData : public QDialog
{
    Q_OBJECT

public:
    ImportData(const QString &file, QWidget *parent = 0);
    ImportData(QWidget *parent = 0);
    ~ImportData();

    inline DataClass getStoredData(){ return *m_storeddata;}
    
private:
    void setUi();
    void WriteData(const DataTable *model);
    QPointer<TableView > m_table;
    QPointer<QLineEdit > m_line;
    QPointer<QPushButton > m_select, m_export, m_file;
    QPointer<QSpinBox > m_conc, m_sign;
    QPointer<QDialogButtonBox > m_buttonbox;
    QPointer<DataClass > m_storeddata;
    QPointer<QCheckBox > m_switch_concentration;
    QString m_filename;
    
private slots:
    void LoadFile();
    void SelectFile();
    void ExportFile();
    void accept();
    void NoChanged();
};

#endif // IMPORTDATA_H
