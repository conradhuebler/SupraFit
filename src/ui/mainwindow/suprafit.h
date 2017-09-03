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
#include "src/ui/mainwindow/modelhistorywidget.h"

#include <QtCore/QFile>
#include <QtCore/QPointer>


#include <QtWidgets/QMainWindow>
#include <QtWidgets/QSplitter>


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
    
    
    bool SetData(QPointer<const DataClass > dataclass, const QString &str = QString("new table"));
    
    
public slots:
    void LoadFile(const QString &file);
    
private:
    void setActionEnabled(bool enabled);
    void ReadSettings();
    void ReadGeometry();
    void WriteSettings(bool ignore_window_state = true);
    
    bool LoadProject(const QString &filename);
    void ImportTable(const QString &file);
    
    QIcon Icon(const QString &str);
    
    QPointer<QSplitter >m_mainsplitter;
    QPointer<ChartWidget > m_charts;
    QPointer<ModelDataHolder > m_model_dataholder;
    QPointer<ModelHistory> m_historywidget;
    QToolBar *m_main_toolbar, *m_model_toolbar, *m_system_toolbar;
    QSharedPointer <DataClass > m_titration_data;
    bool m_hasData;
    QAction *m_new, *m_edit, *m_config, *m_about, *m_aboutqt, *m_close, *m_export, *m_save, *m_load, *m_importmodel;
    OptimizerConfig m_opt_config;

    QDockWidget *m_logdock, *m_modeldock, *m_chartdock, *m_history_dock;
    QPlainTextEdit *m_logWidget;
    QString m_logfile;
    int m_printlevel;
    void LogFile();
    QFile m_file, m_stdout;
    virtual void closeEvent(QCloseEvent *event);
    const QStringList m_properties = QStringList() << "threads" << "chartanimation" << "workingdir" << "dirlevel" << "auto_confidence" << "lastdir" << "p_value" << "charttheme" << "ask_on_exit" << "tooltips";
    
private slots:
    void NewTable();
    void OpenFile();
    void setWindowTitle();
    void SaveProjectAction();
    void ImportModelAction();
    void ExportModelAction();
    void SettingsDialog();
    void about();
    void WriteMessages(const QString &message, int priority);
    void MessageBox(const QString &str, int priority);
    void InsertHistoryElement(const QJsonObject &model);
    void InsertHistoryElement(const QJsonObject &model, int active);
    void FirstStart();
    
signals:
    void AppendPlainText(const QString &str);
    
   protected:
       bool eventFilter(QObject *obj, QEvent *ev);
};

#endif // nmr2fit_H
