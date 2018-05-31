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

#include <QtCore/QAbstractItemModel>
#include <QtCore/QFile>
#include <QtCore/QPointer>

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QSplitter>

class QDockWidget;
class QGridLayout;
class QListWidget;
class QResizeEvent;
class QPlainTextEdit;
class QToolBar;
class QTabWidget;
class QTreeView;

class DataClass;
class ChartWidget;

class Instance;
class MainWindow;
class ModelDataHolder;

struct OptimizerConfig;

class ProjectTree : public QAbstractItemModel {
    Q_OBJECT
public:
    inline ProjectTree(QVector<QPointer<MainWindow>>* project_list) { m_project_list = project_list; }

    inline ~ProjectTree() {}

    Qt::ItemFlags flags(const QModelIndex& index) const override
    {
        Q_UNUSED(index);
        Qt::ItemFlags flags;
        //if (m_checkable)
        flags = Qt::ItemIsDragEnabled | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable;
        //else
        //    flags = Qt::ItemIsEnabled;

        return flags;
    }

    /* Qt::DropActions supportedDragActions() const override
    {

    }*/

    virtual int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    virtual int rowCount(const QModelIndex& parent = QModelIndex()) const override;

    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;

    virtual QModelIndex parent(const QModelIndex& child) const override;

private:
    QVector<QPointer<MainWindow>>* m_project_list;
};

class SupraFitGui : public QMainWindow {
    Q_OBJECT
public:
    explicit SupraFitGui();
    ~SupraFitGui();

    bool SetData(const QJsonObject& object, const QString& file);

public slots:
    void LoadFile(const QString& file);

private:
    void LoadJson(const QJsonObject& json);
    void setActionEnabled(bool enabled);
    void ReadSettings();
    void ReadGeometry();
    void WriteSettings(bool ignore_window_state = true);

    bool LoadProject(const QString& filename);
    void ImportTable(const QString& file);

    QToolBar *m_main_toolbar, *m_model_toolbar, *m_system_toolbar;
    QVector<QWeakPointer<DataClass>> m_data_list;
    bool m_hasData;
    QAction *m_new_window, *m_new_table, *m_config, *m_about, *m_aboutqt, *m_close, *m_save, *m_load;
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
    QVector<QPointer<MainWindow>> m_project_list;
    QTreeView* m_project_view;
    QPointer<ProjectTree> m_project_tree;
    QGridLayout* m_layout;
    QWeakPointer<MetaModel> m_meta_model;

private slots:
    void NewWindow();
    void NewTable();
    void OpenFile();
    void setWindowTitle();
    void SaveProjectAction();
    void SettingsDialog();
    void about();
    void WriteMessages(const QString& message, int priority);
    void MessageBox(const QString& str, int priority);
    void FirstStart();

    void AddMetaModel(const QModelIndex& index);
    void UpdateTreeView(bool regenerate = false);
    void TreeClicked(const QModelIndex& index);
    void TreeRemoveRequest(const QModelIndex& index);

signals:
    void AppendPlainText(const QString& str);

protected:
    bool eventFilter(QObject* obj, QEvent* ev);
};
