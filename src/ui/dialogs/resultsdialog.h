/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <QtGui/QStandardItem>

#include <QtWidgets/QDialog>

#include "src/global.h"

class QGridLayout;
class QListView;
class QListWidgetItem;
class QStandardItemModel;
class QSplitter;
class QTabWidget;
class QToolButton;

class AbstractModel;
class ChartWrapper;
class ResultsWidget;

class ResultsDialog : public QDialog {
    Q_OBJECT
public:
    ResultsDialog(QSharedPointer<AbstractModel> model, ChartWrapper* wrapper, QWidget* parent);
    virtual ~ResultsDialog() override;

    /*! \brief One finished analysis of the model as listed in the dialog. */
    struct Entry {
        QString title;
        SupraFit::Method type;
        int index;
    };
    QList<Entry> Entries() const;

    /*! \brief Use the dialog as a page of a model pane: the pane lists the results itself, so the list inside
     * is hidden, Escape does not close it and Attention() asks the pane to show it. Claude Generated. */
    void setEmbedded(bool embedded);
    void reject() override;

    bool ShowEntry(SupraFit::Method type, int index); ///< show (open on first use) the result's tab
    void SaveEntry(SupraFit::Method type, int index);
    void DropRawDataOf(SupraFit::Method type, int index);
    void RemoveEntry(SupraFit::Method type, int index);

    /*! \brief Take the result out of the pane into a window of its own (a second call raises it). Closing that
     * window puts the result back as a tab, so a result never gets lost by closing. Claude Generated. */
    bool DetachEntry(SupraFit::Method type, int index);
    void DetachCurrent(); ///< the result shown in the tab view

public slots:
    void ShowResult(SupraFit::Method type, int index);
    void Attention();

private:
    inline QString Index(const QStandardItem* item) const { return QString::number(item->data(Qt::UserRole).toInt()) + ":" + QString::number(item->data(Qt::UserRole + 1).toInt()); }
    inline QString Index(int type, int index) const { return QString::number(type) + ":" + QString::number(index); }

    QWeakPointer<AbstractModel> m_model;
    QSplitter* m_mainwidget;
    QListView* m_results;
    QStandardItemModel* m_itemmodel = nullptr;
    QTabWidget* m_tabs;
    QGridLayout* m_layout;
    ChartWrapper* m_wrapper;
    QHash<QString, int> m_indices;
    QHash<double, ResultsWidget*> m_stored_widgets;
    QAction *m_load, *m_save, *m_drop_data, *m_remove;
    bool m_embedded = false;
    QHash<double, QDialog*> m_detached; ///< result timestamp -> window the result lives in
    QToolButton* m_detach = nullptr;

    QModelIndex IndexOf(SupraFit::Method type, int index) const;

private slots:
    void UpdateList();
    void itemDoubleClicked(const QModelIndex& index);
    void RemoveItem(const QModelIndex& index);
    void Save(const QModelIndex& index);
    void DropRawData(const QModelIndex& index);

signals:
    void AttentionRequested(); ///< embedded: the pane should show the results
    void ListChanged(); ///< the list of finished analyses changed
    void LoadModel(const QJsonObject& object);
    void AddModel(const QJsonObject& object);
};
