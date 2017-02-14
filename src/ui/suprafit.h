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
#include "src/ui/widgets/optimizerwidget.h"
#include "src/ui/widgets/modelhistorywidget.h"
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QSplitter>
#include <QtCore/QPointer>
#include <QtCharts/QLineSeries>
#include <QtCore/QFile>
struct OptimizerConfig;

class ModelDataHolder;
class ChartWidget;
class QResizeEvent;
class QListWidget;
class DataClass;
class QToolBar;
class QDockWidget;
class QPlainTextEdit;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();
    virtual ~MainWindow();
    void ImportAction(const QString &file);
    bool SetData(QPointer<const DataClass > dataclass, const QString &str = QString("new table"));
    
    
public slots:

    
private:
    void setActionEnabled(bool enabled);
    QPointer<QSplitter >m_mainsplitter;
    QPointer<ChartWidget > m_charts;
    QPointer<ModelDataHolder > m_model_dataholder;
    QPointer<ModelHistory> m_historywidget;
    QToolBar *m_main_toolbar, *m_model_toolbar, *m_system_toolbar;
    QSharedPointer <DataClass > m_titration_data;
    bool m_hasData;
    QAction *m_new, *m_import, *m_edit, *m_config, *m_about, *m_close, *m_export, *m_save, *m_load, *m_importmodel;
    OptimizerConfig m_opt_config;
    void ReadSettings();
    void WriteSettings(bool ignore_window_state = true);
    QDockWidget *m_logdock, *m_modeldock, *m_chartdock, *m_history_dock;
    QPlainTextEdit *m_logWidget;
    QString m_logfile;
    int m_printlevel;
    void LogFile();
    QFile m_file, m_stdout;
    QMap<int, ModelHistoryElement> m_history;
    bool m_ask_on_exit;
    virtual void closeEvent(QCloseEvent *event);
private slots:
    void NewTableAction();
    void ImportTableAction();
    void EditTableAction();
    void LoadProjectAction();
    void SaveProjectAction();
    void ImportModelAction();
    void ExportModelAction();
    void SettingsDialog();
    void WriteMessages(const QString &message, int priority);
    void MessageBox(const QString &str, int priority);
    void InsertHistoryElement(const ModelHistoryElement &element);
    void InsertHistoryElement(const QJsonObject &model);
signals:
    void AppendPlainText(const QString &str);
};

#endif // nmr2fit_H