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

#include "src/ui/mainwindow/mainwindow.h"
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
class QTabWidget;

class DataClass;
class ChartWidget;

class Instance;
class MainWindow;
class ModelDataHolder;

struct OptimizerConfig;

class SupraFitGui : public QMainWindow {
    Q_OBJECT
public:
    explicit SupraFitGui();
    ~SupraFitGui();

    bool SetData(QPointer<const DataClass> dataclass, const QString& str = QString("new table"), const QString& colors = QString());

public slots:
    void LoadFile(const QString& file);

private:
    void setActionEnabled(bool enabled);
    void ReadSettings();
    void ReadGeometry();
    void WriteSettings(bool ignore_window_state = true);

    bool LoadProject(const QString& filename);
    void ImportTable(const QString& file);

    QIcon Icon(const QString& str);

    QToolBar *m_main_toolbar, *m_model_toolbar, *m_system_toolbar;
    QVector<QSharedPointer<DataClass>> m_data_list;
    bool m_hasData;
    QAction *m_new, *m_edit, *m_config, *m_about, *m_aboutqt, *m_close, *m_export, *m_save, *m_load, *m_importmodel;
    OptimizerConfig m_opt_config;

    QString m_logfile;
    int m_printlevel;
    void LogFile();
    QFile m_file, m_stdout;
    virtual void closeEvent(QCloseEvent* event);
    const QStringList m_properties = QStringList() << "threads"
                                                   << "chartanimation"
                                                   << "workingdir"
                                                   << "dirlevel"
                                                   << "auto_confidence"
                                                   << "lastdir"
                                                   << "p_value"
                                                   << "charttheme"
                                                   << "ask_on_exit"
                                                   << "tooltips";

    QPointer<Instance> m_instance;
    QTabWidget* m_central_widget;

private slots:
    void NewTable();
    void OpenFile();
    void setWindowTitle();
    void SaveProjectAction();
    void ImportModelAction();
    void ExportModelAction();
    void SettingsDialog();
    void about();
    void WriteMessages(const QString& message, int priority);
    void MessageBox(const QString& str, int priority);
    void InsertHistoryElement(const QJsonObject& model);
    void InsertHistoryElement(const QJsonObject& model, int active);
    void FirstStart();
    void EditData();

signals:
    void AppendPlainText(const QString& str);

protected:
    bool eventFilter(QObject* obj, QEvent* ev);
};
