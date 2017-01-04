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

#ifndef MODELDATAHOLDER_H
#define MODELDATAHOLDER_H
#include "src/core/dataclass.h"
#include <QtCharts/QChart>
#include <QtWidgets/QWidget>
#include <QtCore/QPointer>

struct OptimizerConfig;

class ModelWidget;
class DataWidget;
class QTabWidget;
class QPushButton;
class ChartWidget;
class QPlainTextEdit;
class ModelDataHolder : public QWidget
{
    Q_OBJECT

public:
    ModelDataHolder();
    ~ModelDataHolder();
    void setData(DataClass *data);
    void setChartWidget(const QPointer<ChartWidget> chart) { m_charts = chart; }
    enum {
        ItoI = 1,
        IItoI_ItoI = 2,
        ItoI_ItoII = 3,
        IItoI_ItoI_t = 4
    };
    void setSettings(const OptimizerConfig &config);
private:
    QPointer<DataWidget > m_datawidget;
    QPointer<QTabWidget > m_modelsWidget;
    QPointer<QPushButton > m_add, m_simulate;
    QPointer<ChartWidget> m_charts;
    DataClass *m_data;
//     QPlainTextEdit *m_logWidget;
    QVector<QPointer< AbstractTitrationModel > > m_models;
    void AddModel(int model);
    void SimulateModel(int model);
    OptimizerConfig m_config;
private slots:
    void AddModel11();
    void AddModel21();
    void AddModel21_t();
    void AddModel12();
    void SimulateModel11();
    void SimulateModel21();
    void SimulateModel12();
    void RemoveTab(int i);
    
signals:
    void ModelAdded(AbstractTitrationModel *model);
    void Message(const QString &str, int priority);
    void MessageBox(const QString &str, int priority);

};

#endif // MODELDATAHOLDER_H
