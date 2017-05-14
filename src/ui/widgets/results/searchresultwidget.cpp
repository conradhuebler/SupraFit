/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2017  Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/capabilities/globalsearch.h"
#include "src/core/toolset.h"
#include "src/core/jsonhandler.h"
#include "src/core/models.h"
#include "src/ui/widgets/buttons/scientificbox.h"
#include "src/ui/widgets/chartview.h"

#include <QtCore/QJsonObject>
#include <QtCore/QSharedPointer>
#include <QtCore/QCollator>
#include <QtCore/QFile>

#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <QtWidgets/QTableView>
#include <QtWidgets/QWidget>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QFileDialog>

#include <QtCharts/QScatterSeries>
#include <QtCharts/QChart>

#include "searchresultwidget.h"

SearchResultWidget::SearchResultWidget(QPointer<GlobalSearch> globalsearch, const QSharedPointer<AbstractModel> model, QWidget *parent) : QWidget(parent), m_globalsearch(globalsearch), m_model(model)
{    
    setInputList(m_globalsearch->InputList());
    setModelList(m_globalsearch->Models());
    
    QGridLayout *layout = new QGridLayout;
    m_switch = new QPushButton(tr("Switch View"));
    m_export = new QPushButton(tr("Export Models"));
    m_valid = new QCheckBox(tr("Invalid Models"));
    m_threshold = new ScientificBox;
    m_threshold->setValue(1);
    layout->addWidget(m_switch, 0, 0);
    layout->addWidget(new QLabel(tr("Threshold SSE")), 0, 1);
    layout->addWidget(m_threshold, 0, 2);
    layout->addWidget(m_valid, 0, 3);
    layout->addWidget(m_export, 0, 4);
    
    if(!m_model)
        throw 1;
    
    m_table = BuildList();
    m_table->setSortingEnabled(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_table, SIGNAL(clicked(QModelIndex)), this, SLOT(rowSelected(QModelIndex)));
    connect(m_table, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(ShowContextMenu(const QPoint&)));
    connect(m_export, SIGNAL(clicked()), this, SLOT(ExportModels()));
    connect(m_switch, SIGNAL(clicked()), this, SLOT(SwitchView()));
    layout->addWidget(m_table, 1, 0, 1, 5);
    
    m_contour = BuildContour();
    layout->addWidget(m_contour, 1, 0, 1, 5);
    m_contour->hide();
    setLayout(layout);
}

SearchResultWidget::~SearchResultWidget()
{
    m_models.clear();
}

QTableView* SearchResultWidget::BuildList()
{
    QTableView *table = new QTableView(this);
    QStandardItemModel *model = new QStandardItemModel;
    QStringList header = QStringList() <<  "Sum of Squares";
    for(int i = 0; i < m_models.size(); ++i)
    {
        double error = m_models[i]["sum_of_squares"].toDouble();
        QStandardItem *item = new QStandardItem(QString::number(error));
        item->setData(i, Qt::UserRole);
        model->setItem(i, 0, item);
        int j = 1;
        for(int l = 0; l < m_input[i].size(); ++l)
        {
            QStandardItem *item = new QStandardItem(QString::number(m_input[i][l]));
            item->setData(i, Qt::UserRole);
            model->setItem(i, j, item);
            j++;
        }
        
        QJsonObject constants = m_models[i]["data"].toObject()["constants"].toObject();
        QStringList keys = constants.keys();
        
        if(keys.size() > 10)
        {
            QCollator collator;
            collator.setNumericMode(true);
            std::sort(
                keys.begin(),
                      keys.end(),
                      [&collator](const QString &key1, const QString &key2)
                      {
                          return collator.compare(key1, key2) < 0;
                      });
        }
        
        QString consts;
        
        for(const QString &str : qAsConst(keys))
        {
            QString element = constants[str].toString();
            if(!element.isNull() && !element.isEmpty())
            {
                QStandardItem *item = new QStandardItem(element);
                item->setData(i, Qt::UserRole);
                model->setItem(i, j, item);
                j++;
            }
            
        }
    }
    QStringList head;
    for(const QString &str : m_model.data()->GlobalParameterNames())
        head << str;
    header << head << head;
    model->setHorizontalHeaderLabels(header);
    QSortFilterProxyModel *proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(model);
    table->setModel(proxyModel);
    table->resizeColumnsToContents();
    resize(table->sizeHint());
    return table;
}

ChartView * SearchResultWidget::BuildContour()
{
    QList<QPointF > data = ToolSet::fromModelsList(m_models);
    QtCharts::QChart *chart_ellipsoid = new QtCharts::QChart;
    chart_ellipsoid->setAnimationOptions(QtCharts::QChart::SeriesAnimations);
    ChartView *view = new ChartView(chart_ellipsoid);
    QtCharts::QScatterSeries *xy_series = new QtCharts::QScatterSeries(this);
    xy_series->append(data);
    xy_series->setMarkerSize(8);
    view->addSeries(xy_series);
    return view;
}


void SearchResultWidget::rowSelected(const QModelIndex &index)
{
    int i = index.data(Qt::UserRole).toInt();
    QJsonObject model = m_models[i];
    emit LoadModel(model);
}

void SearchResultWidget::ShowContextMenu(const QPoint& pos)
{
    Q_UNUSED(pos)
    QModelIndex index = m_table->currentIndex();
    int i = index.data(Qt::UserRole).toInt();
    QJsonObject model = m_models[i];
    emit AddModel(model);
}

void SearchResultWidget::ExportModels()
{
    qreal threshold = m_threshold->value();
    bool allow_invalid = m_valid->isChecked();
    QString str = QFileDialog::getSaveFileName(this, tr("Save File"), getDir(), tr("Json File (*.json);;Binary (*.jdat);;All files (*.*)" ));
    if(str.isEmpty())
        return;
    setLastDir(str);
    m_globalsearch->ExportResults(str, threshold, allow_invalid);
    
}

void SearchResultWidget::SwitchView()
{
    bool histogram = m_table->isHidden();
    m_table->setHidden(!histogram);
    m_contour->setHidden(histogram);
}

#include "searchresultwidget.h"
