/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/ui/guitools/mime.h"
#include "src/core/models/meta_model.h" // MetaModel class (was transitive via models.h)
#include "src/ui/mainwindow/mainwindow.h"
#include "src/ui/widgets/optimizerwidget.h"
#include "src/core/projectmanager.h"

#include <QtCore/QAbstractItemModel>
#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QMimeData>
#include <QtCore/QModelIndex>
#include <QtCore/QPointer>
#include <QtCore/QUuid>

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QSplitter>

class QDockWidget;
class QGridLayout;
class QListWidget;
class QResizeEvent;
class QPlainTextEdit;
class QPropertyAnimation;
class QPushButton;
class QSplashScreen;
class QSplitter;
class QStackedWidget;
class QToolBar;
class QTabWidget;
class QTreeView;
class QSystemTrayIcon;

class DataClass;
class ChartWidget;
class MessageDock;
class Instance;
class MainWindow;
class ModelDataHolder;
class ProjectTree;

struct OptimizerConfig;

class DropButton : public QToolButton {
    Q_OBJECT
public:
    DropButton()
    {
        setAcceptDrops(true);
    }

protected:
    void dropEvent(QDropEvent* event) override
    {
        const QMimeData* data = event->mimeData();
        QByteArray sprmodel = data->data("application/x-suprafitmodel");

        QJsonDocument doc = QJsonDocument::fromJson(sprmodel);

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
    void DataDropped(const QJsonObject& data);
};

class LogoLabel : public QLabel {
    Q_OBJECT
public:
    LogoLabel(QWidget* parent)
        : QLabel(parent)
    {
    }
    virtual ~LogoLabel() {}

    void setPixmap(const QString& pixmap)
    {
        m_pixmap = pixmap;
        UpdatePixmap();
    }

    void UpdatePixmap()
    {
        if (m_pixmap.isEmpty() || m_pixmap.isNull())
            return;
        QPixmap p(m_pixmap);
        int w = width();
        int h = height();
        QLabel::setPixmap(p.scaled(w, h, Qt::KeepAspectRatio));
    }
    void resizeEvent(QResizeEvent* event) override
    {
        Q_UNUSED(event)
        UpdatePixmap();
    }

private:
    QString m_pixmap;
    QPixmap m_pix;
};

class SupraFitGui : public QMainWindow {
    Q_OBJECT
public:
    explicit SupraFitGui();
    virtual ~SupraFitGui() override;

    [[deprecated("Use ProjectManager to load and create projects")]]
    bool SetData(const QJsonObject& object, const QString& file, const QString& path);
    virtual QSize sizeHint() const override { return QSize(400, 300); }

public slots:
    void LoadFile(const QString& file, int overwrite_type = 0);
    void SpectraEdited(const QJsonObject& table, const QJsonObject& data);

private:
    // This will become an improved data load mechanism somedays

    // QVector<QJsonObject> ProjectFromFile(const QString& file);
    // QVector<QJsonObject> ProjectFromFiles(const QStringList& files);

    void LoadJson(const QJsonObject& str);
    void setActionEnabled(bool enabled);
    void ReadSettings();
    void ReadGeometry();
    void WriteSettings(bool ignore_window_state = true);

    void Message(const QString& str);
    void Info(const QString& str);
    void Warning(const QString& str);

    bool LoadProject(const QString& filename);
    void ImportTable(const QString& file);
    void LoadMetaModels();

    QPair<int, int> UUID2Widget(const QModelIndex& index);

    void AddScatter(const QJsonObject& data);

    QToolBar *m_main_toolbar, *m_model_toolbar, *m_system_toolbar;
    QVector<QWeakPointer<DataClass>> m_data_list;
    QHash<QString, QWeakPointer<DataClass>> m_hashed_data;
    QHash<QString, QWeakPointer<ChartWrapper>> m_hashed_wrapper;
    // Claude Generated - ProjectManager integration: UUID to MainWindow mapping
    QHash<QString, QPointer<MainWindow>> m_project_windows;
    bool m_hasData;
    QAction *m_new_window, *m_new_table, *m_spectra, *m_thermogram, *m_config, *m_about, *m_aboutqt, *m_message_dock_action, *m_close, *m_save, *m_save_as, *m_load, *m_license, *m_project_action;
    QJsonObject m_opt_config;

    int m_last_index = -1, m_project_tree_size;
    bool m_alert = false;

    QString m_supr_file;

    QFile m_file;
    virtual void closeEvent(QCloseEvent* event) override;

    QDockWidget* m_message_dock;
    MessageDock* m_messages_widget;
    QPointer<Instance> m_instance;
    QTabWidget* m_central_widget;
    QWidget *m_project_holder, *m_recentWidget;
    QPropertyAnimation *m_show_tree, *m_show_dock;
    QListWidget *m_recent_documents;
    // Claude Generated - REMOVED: QVector<QPointer<MainWindow>> m_project_list; - replaced by m_project_windows UUID-based mapping
    QTreeView* m_project_view;
    QPointer<ProjectTree> m_project_tree;
    QStackedWidget* m_stack_widget;
    QVector<QWeakPointer<MetaModel>> m_meta_models;
    QSplashScreen* m_splash;
    QSplitter *m_mainsplitter, *m_blank_widget;
    QVector<QJsonObject> m_cached_meta;
    QLineEdit* m_filename_line;

    QPushButton* m_clear_recent;
    QToolButton *m_export_suprafit, *m_export_plain, *m_close_all;
    DropButton* m_add_scatter;
    QString m_supported_files = QString("Supported files (*.suprafit *.json *.jdat *.txt *.dat *.itc *.ITC *.dh *.DH *.csv);;Json File (*.json);;SupraFit Project File  (*.suprafit);;Table Files (*.dat *.txt *.csv *.itc *.ITC);;Origin Files(*.dh *.DH);;All files (*.*)");
    LogoLabel* m_logolabel;

private slots:
    void NewWindow();
    void NewTable();
    void OpenFile();
    void OpenThermogram();
    void OpenSpectraDir();
    void setWindowTitle();
    void SaveProjectAction();
    void SaveAsProjectAction();
    void SettingsDialog();
    void about();
    void LicenseInfo();

    void FirstStart();
    void UpdateRecentList();
    void AddMetaModel(const QModelIndex& index, int position);
    void CopySystemParameter(const QModelIndex& source, int position);

    void SaveData(const QModelIndex& index);
    void AddUpData(const QModelIndex& index, bool sign);
    void Duplicate(const QModelIndex& index);

    void CopyModel(const QJsonObject& o, int data, int model);

    void TreeDoubleClicked(const QModelIndex& index);
    void TreeClicked(const QModelIndex& index);
    void TreeRemoveRequest(const QModelIndex& index);
    void CloseProjects();

    void ExportAllPlain();
    void ExportAllSupraFit();
    void AddScatter();
    
    // Claude Generated - ProjectManager Integration Slots
    void onProjectLoaded(const QString& projectId, const QString& filePath);
    void onProjectSaved(const QString& projectId, const QString& filePath);
    void onModelAdded(const QString& projectId, const QString& modelId);
    void onProjectManagerError(const QString& operation, const QString& errorMessage);
    void onProjectAdded(const QString& projectId, const QString& projectTitle);
    void onCurrentProjectChanged(const QString& projectId);
    void onProjectDataUpdated(const QString& projectId);

private:
    // Claude Generated - Removed updateDataListFromProjectManager() function declaration - no longer needed

protected:
    virtual bool eventFilter(QObject* obj, QEvent* ev) override;
    virtual void resizeEvent(QResizeEvent* event) override;
};
