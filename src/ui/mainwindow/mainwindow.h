/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016 - 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#pragma once

#include "src/ui/mainwindow/modeldataholder.h"
#include "src/ui/mainwindow/modelhistorywidget.h"
#include "src/ui/widgets/optimizerwidget.h"

#include <QtCore/QFile>
#include <QtCore/QPointer>

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QSplitter>

class QDockWidget;
class QListWidget;
class QResizeEvent;
class QPlainTextEdit;
class QToolBar;

class DataClass;
class ChartWidget;

class Instance;
class ModelDataHolder;

struct OptimizerConfig;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow();
    ~MainWindow();

    QSharedPointer<DataClass> SetData(const QJsonObject& object);
    QSharedPointer<AbstractModel> CreateMetaModel();

    inline QJsonObject SaveProject() const { return m_model_dataholder->SaveWorkspace(); }
    inline QJsonObject SaveModel(int index) const { return m_model_dataholder->SaveModel(index); }

    inline int ModelCount() const { return m_model_dataholder->ModelCount(); }

    inline QPointer<AbstractModel> Model(int index) const { return m_model_dataholder->Model(index); }

    inline QPointer<DataClass> Data() const { return m_data.data(); }

    void setCurrentTab(int index);

    inline void RemoveTab(int i) { m_model_dataholder->RemoveTab(i); }

    inline bool isMetaModel() const { return m_meta_model; }
public slots:
    // void LoadFile(const QString& file);

private:
    void setActionEnabled(bool enabled);
    void ReadSettings();
    void ReadGeometry();
    void WriteSettings(bool ignore_window_state = true);

    QPointer<QSplitter> m_mainsplitter;
    QPointer<ChartWidget> m_charts;
    QPointer<ModelDataHolder> m_model_dataholder;
    QPointer<ModelHistory> m_historywidget;
    QToolBar *m_main_toolbar, *m_model_toolbar, *m_system_toolbar;
    QSharedPointer<DataClass> m_data;
    bool m_hasData, m_meta_model = false;
    QAction *m_new, *m_edit, *m_config, *m_about, *m_aboutqt, *m_close, *m_export, *m_save, *m_load, *m_importmodel;
    OptimizerConfig m_opt_config;

    QDockWidget *m_modeldock, *m_chartdock, *m_history_dock;
    QString m_logfile;
    int m_printlevel;
    void LogFile();
    QFile m_file, m_stdout;

private slots:
    void InsertHistoryElement(const QJsonObject& model);
    void InsertHistoryElement(const QJsonObject& model, int active);
    void EditData();

signals:
    void AppendPlainText(const QString& str);
    void ModelAdded();
    void ModelsChanged();
};
