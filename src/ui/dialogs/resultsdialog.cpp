/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2018 - 2019 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <QtCore/QWeakPointer>

#include <QtGui/QStandardItemModel>

#include <QtWidgets/QAction>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QListView>
#include <QtWidgets/QSplitter>

#include "src/global.h"

#include "src/core/jsonhandler.h"
#include "src/core/models.h"

#include "src/ui/guitools/chartwrapper.h"
#include "src/ui/guitools/instance.h"

#include "src/ui/widgets/results/resultswidget.h"

#include "resultsdialog.h"

ResultsDialog::ResultsDialog(QSharedPointer<AbstractModel> model, ChartWrapper* wrapper, QWidget* parent)
    : QDialog(parent)
    , m_model(model)
    , m_wrapper(wrapper)
{
    m_layout = new QGridLayout;
    setModal(false);
    setWindowFlags(Qt::Window);
    m_mainwidget = new QSplitter(Qt::Horizontal);
    m_layout->addWidget(m_mainwidget, 0, 0);

    m_results = new QListView;
    m_results->setMaximumWidth(200);
    m_results->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_results->setContextMenuPolicy(Qt::ActionsContextMenu);
    connect(m_results, &QListView::doubleClicked, this, &ResultsDialog::itemDoubleClicked);

    QAction* action;
    action = new QAction("Load Load", m_results);
    action->setIcon(Icon("window-new"));
    m_results->addAction(action);
    connect(action, &QAction::triggered, action, [this]() {
        itemDoubleClicked(m_results->currentIndex());
    });

    action = new QAction("Save data", m_results);
    m_results->addAction(action);
    action->setIcon(Icon("document-save-as"));
    connect(action, &QAction::triggered, action, [this]() {
        Save(m_results->currentIndex());
    });

    action = new QAction("Remove Item", m_results);
    m_results->addAction(action);
    action->setIcon(Icon("trash-empty"));
    connect(action, &QAction::triggered, action, [this]() {
        RemoveItem(m_results->currentIndex());
    });

    m_tabs = new QTabWidget;
    //m_tabs->setTabsClosable(true);
    //m_tabs->setMovable(true);

    m_mainwidget->addWidget(m_results);
    m_mainwidget->addWidget(m_tabs);

    setWindowTitle("Collected Results for " + m_model.data()->Name());
    setLayout(m_layout);
    resize(1024, 800);
    UpdateList();
    connect(m_model.data(), &AbstractModel::StatisticChanged, this, &ResultsDialog::UpdateList);
}

ResultsDialog::~ResultsDialog()
{
    m_model.clear();
}

void ResultsDialog::Attention()
{
    show();
    raise();
    activateWindow();
}

void ResultsDialog::ShowResult(SupraFit::Method type, int index)
{
    ResultsWidget* results = new ResultsWidget(m_model.data()->getStatistic(type, index), m_model, m_wrapper);
    QString name = tr("%1 # %2").arg(SupraFit::Method2Name(type)).arg(index + 1);
    int tab = m_tabs->addTab(results, name);

    connect(results, &ResultsWidget::LoadModel, this, &ResultsDialog::LoadModel);
    connect(results, &ResultsWidget::AddModel, this, &ResultsDialog::AddModel);
    m_stored_widgets[name] = results;

    m_tabs->setCurrentIndex(tab);
    m_indices[Index(type, index)] = tab;
}

void ResultsDialog::UpdateList()
{
    auto makeItem = [this](QStandardItem* item) {
        const QString index = Index(item);
        if (m_indices.keys().contains(index))
            item->setData(m_indices[index], Qt::UserRole + 2);
        else {
            item->setData(-1, Qt::UserRole + 2);
            m_indices[index] = -1;
        }
    };

    auto addItem = [this, makeItem](QStandardItem* item, const QJsonObject& object, int i) {
        QJsonObject controller = object["controller"].toObject();
        item->setData(tr("%1 # %2").arg(SupraFit::Method2Name(controller["method"].toInt())).arg(i + 1), Qt::DisplayRole);
        item->setData(controller["method"].toInt(), Qt::UserRole);
        item->setData(i, Qt::UserRole + 1);
        makeItem(item);
    };

    QStandardItemModel* model = new QStandardItemModel(this);
    for (int i = 0; i < m_model.data()->getMCStatisticResult(); ++i) {
        QStandardItem* item = new QStandardItem;
        addItem(item, m_model.data()->getStatistic(SupraFit::Method::MonteCarlo, i), i);
        model->appendRow(item);
    }

    for (int i = 0; i < m_model.data()->getCVStatisticResult(); ++i) {
        QJsonObject controller = m_model.data()->getStatistic(SupraFit::Method::CrossValidation, i)["controller"].toObject();
        if (controller.isEmpty())
            continue;
        QStandardItem* item = new QStandardItem;
        addItem(item, m_model.data()->getStatistic(SupraFit::Method::CrossValidation, i), i);
        model->appendRow(item);
    }

    for (int i = 0; i < m_model.data()->getWGStatisticResult(); ++i) {
        QJsonObject controller = m_model.data()->getStatistic(SupraFit::Method::WeakenedGridSearch, i)["controller"].toObject();
        if (controller.isEmpty())
            continue;

        QStandardItem* item = new QStandardItem;
        addItem(item, m_model.data()->getStatistic(SupraFit::Method::WeakenedGridSearch, i), i);
        model->appendRow(item);
    }

    for (int i = 0; i < m_model.data()->getMoCoStatisticResult(); ++i) {
        QJsonObject controller = m_model.data()->getStatistic(SupraFit::Method::ModelComparison, i)["controller"].toObject();
        if (controller.isEmpty())
            continue;

        QStandardItem* item = new QStandardItem;
        addItem(item, m_model.data()->getStatistic(SupraFit::Method::ModelComparison, i), i);
        model->appendRow(item);
    }

    for (int i = 0; i < m_model.data()->getReductionStatisticResults(); ++i) {
        QStandardItem* item = new QStandardItem;
        addItem(item, m_model.data()->getStatistic(SupraFit::Method::Reduction, i), i);
        model->appendRow(item);
    }

    for (int i = 0; i < m_model.data()->SearchSize(); ++i) {
        QStandardItem* item = new QStandardItem;
        addItem(item, m_model.data()->getStatistic(SupraFit::Method::GlobalSearch, i), i);
        model->appendRow(item);
    }

    m_itemmodel = model;
    m_results->setModel(m_itemmodel);
}

void ResultsDialog::itemDoubleClicked(const QModelIndex& index)
{
    QStandardItem* item = m_itemmodel->itemFromIndex(index);

    if (-1 == m_indices[Index(item)]) {
        SupraFit::Method type = SupraFit::Method(item->data(Qt::UserRole).toInt());
        int index = item->data(Qt::UserRole + 1).toInt();
        ResultsWidget* results = new ResultsWidget(m_model.data()->getStatistic(type, index), m_model, m_wrapper);
        QString name = tr("%1 # %2").arg(SupraFit::Method2Name(type)).arg(index + 1);
        int tab = m_tabs->addTab(results, name);
        connect(results, &ResultsWidget::LoadModel, this, &ResultsDialog::LoadModel);
        connect(results, &ResultsWidget::AddModel, this, &ResultsDialog::AddModel);
        m_stored_widgets[name] = results;
        item->setData(name, Qt::UserRole + 3);
        m_indices[Index(item)] = tab;
    } else
        m_tabs->setCurrentIndex(m_indices[Index(item)]);
}

void ResultsDialog::RemoveItem(const QModelIndex& index)
{
    QStandardItem* item = m_itemmodel->itemFromIndex(index);
    QString name = item->data(Qt::DisplayRole).toString();
    if (m_stored_widgets.contains(name)) {
        ResultsWidget* results = m_stored_widgets[name];
        m_tabs->removeTab(m_tabs->indexOf(results));
        delete results;
        m_stored_widgets.remove(name);
    }

    SupraFit::Method type = SupraFit::Method(item->data(Qt::UserRole).toInt());
    int i = item->data(Qt::UserRole + 1).toInt();
    m_model.data()->RemoveStatistic(type, i);
    UpdateList();
}

void ResultsDialog::Save(const QModelIndex& index)
{
    QString filename = QFileDialog::getSaveFileName(this, "Select file", getDir());
    if (filename.isEmpty())
        return;

    QStandardItem* item = m_itemmodel->itemFromIndex(index);

    SupraFit::Method type = SupraFit::Method(item->data(Qt::UserRole).toInt());
    int idx = item->data(Qt::UserRole + 1).toInt();
    QJsonObject blob = m_model.data()->getStatistic(type, idx);

    if (blob.isEmpty())
        return;
    QJsonObject toplevel;
    toplevel["data"] = m_model.data()->ExportData();
    QJsonObject model = m_model.data()->ExportModel(false, false);
    QJsonObject datablob = model["data"].toObject();
    QJsonObject statisticObject;
    statisticObject[QString::number(type) + ":0"] = blob;
    datablob["methods"] = statisticObject;
    model["data"] = datablob;
    toplevel["model_0"] = model;

    if (!JsonHandler::WriteJsonFile(toplevel, filename)) {
        qDebug() << "went wrong";
    }
}
