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


#ifndef nmr2fit_H
#define nmr2fit_H

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QSplitter>
#include <QtCore/QPointer>
#include <QtCharts/QLineSeries>
class ModelDataHolder;
class ChartWidget;
class QResizeEvent;
class QListWidget;
class DataClass;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();
    virtual ~MainWindow();
    
    
    
    
    
private:
    QPointer<QSplitter >m_mainsplitter;
    QPointer<ChartWidget > m_charts;
    QPointer<ModelDataHolder > m_model_dataholder;
    QVector <DataClass *> m_loaded_datas;
private slots:
    void LoadData();
    void ImportAction();
protected:
    void resizeEvent(QResizeEvent *event);
};

#endif // nmr2fit_H
