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
#include <QtCore/QJsonDocument>
#include <QtCore/QMimeData>
#include <QtCore/QPointer>

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QSplitter>

class QDockWidget;
class QGridLayout;
class QListWidget;
class QResizeEvent;
class QPlainTextEdit;
class QPushButton;
class QSplashScreen;
class QSplitter;
class QStackedWidget;
class QToolBar;
class QTabWidget;
class QTreeView;

class DataClass;
class ChartWidget;

class Instance;
class MainWindow;
class ModelDataHolder;

struct OptimizerConfig;

class ModelMime : public QMimeData {

    Q_OBJECT

public:
    inline void setData(DataClass* data) { m_data = data; }
    inline DataClass* Data() const { return m_data; }
    inline void setModel(bool model) { m_model = model; }
    inline bool isModel() const { return m_model; }

    inline void setModelIndex(const QModelIndex& index) { m_index = index; }
    inline QModelIndex Index() const { return m_index; }

private:
    DataClass* m_data;
    bool m_model = false;
    QModelIndex m_index;
};

class ProjectTree : public QAbstractItemModel {
    Q_OBJECT
public:
    inline ProjectTree(QVector<QPointer<MainWindow>>* project_list) { m_project_list = project_list; }

    inline ~ProjectTree() {}

    Qt::ItemFlags flags(const QModelIndex& index) const override
    {
        Q_UNUSED(index);
        Qt::ItemFlags flags;

        flags = QAbstractItemModel::flags(index);
        flags = flags | Qt::ItemIsDragEnabled | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsDropEnabled;

        return flags;
    }

    virtual int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    virtual int rowCount(const QModelIndex& parent = QModelIndex()) const override;

    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;

    virtual QModelIndex parent(const QModelIndex& child) const override;

    virtual Qt::DropActions supportedDropActions() const override
    {
        return Qt::CopyAction | Qt::MoveAction;
    }

    QMimeData* mimeData(const QModelIndexList& indexes) const override;

    virtual bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;

    virtual bool canDropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) const override;

private:
    QVector<QPointer<MainWindow>>* m_project_list;

signals:
    void AddMetaModel(const QModelIndex& index, int position);
    void CopySystemParameter(const QModelIndex& source, int position);
    void UiMessage(const QString& str);
    void CopyModel(const QModelIndex& source, int data, int model);
    void LoadFile(const QString& file);
    void LoadJsonObject(const QJsonObject& object);
};

class DropButton : public QPushButton {
    Q_OBJECT
public:
    DropButton(const QString& str)
        : QPushButton(str)
    {
        setAcceptDrops(true);
    }

protected:
    void dropEvent(QDropEvent* event) override
    {
        const QMimeData* data = event->mimeData();
        QByteArray sprmodel = data->data("application/x-suprafitmodel");
        QJsonDocument doc = QJsonDocument::fromBinaryData(sprmodel);

        if (!doc.isEmpty()) {
            emit DataDropped(doc.object());
            return;
        }
    }

    void dragEnterEvent(QDragEnterEvent* event) override
    {
        if (event->mimeData()->hasFormat("application/x-suprafitmodel"))
            event->acceptProposedAction();
    }

signals:
    emit void DataDropped(const QJsonObject& data);
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
    // This will become an improved data load mechanism somedays

    // QVector<QJsonObject> ProjectFromFile(const QString& file);
    // QVector<QJsonObject> ProjectFromFiles(const QStringList& files);

    void LoadJson(const QJsonObject& str);
    void setActionEnabled(bool enabled);
    void ReadSettings();
    void ReadGeometry();
    void WriteSettings(bool ignore_window_state = true);

    bool LoadProject(const QString& filename);
    void ImportTable(const QString& file);
    void LoadMetaModels();

    void AddScatter(const QJsonObject& data);

    QToolBar *m_main_toolbar, *m_model_toolbar, *m_system_toolbar;
    QVector<QWeakPointer<DataClass>> m_data_list;
    QHash<QString, QWeakPointer<DataClass>> m_hashed_data;
    QHash<QString, QWeakPointer<ChartWrapper>> m_hashed_wrapper;
    bool m_hasData;
    QAction *m_new_window, *m_new_table, *m_config, *m_about, *m_aboutqt, *m_close, *m_save, *m_save_as, *m_load;
    OptimizerConfig m_opt_config;

    QString m_supr_file;

    QFile m_file;
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
    QStackedWidget* m_stack_widget;
    QVector<QWeakPointer<MetaModel>> m_meta_models;
    QSplashScreen* m_splash;
    QSplitter* m_mainsplitter;
    QVector<QJsonObject> m_cached_meta;
    QLineEdit* m_filename_line;

    QPushButton *m_export_suprafit, *m_export_plain;
    DropButton* m_add_scatter;

private slots:
    void NewWindow();
    void NewTable();
    void OpenFile();
    void setWindowTitle();
    void SaveProjectAction();
    void SaveAsProjectAction();
    void SettingsDialog();
    void about();
    void FirstStart();

    void AddMetaModel(const QModelIndex& index, int position);
    void CopySystemParameter(const QModelIndex& source, int position);

    void SaveData(const QModelIndex& index);
    void CopyModel(const QModelIndex& source, int data, int model);

    void UpdateTreeView(bool regenerate = false);
    void TreeClicked(const QModelIndex& index);
    void TreeRemoveRequest(const QModelIndex& index);

    void ExportAllPlain();
    void ExportAllSupraFit();
    void AddScatter();

protected:
    bool eventFilter(QObject* obj, QEvent* ev);
};
