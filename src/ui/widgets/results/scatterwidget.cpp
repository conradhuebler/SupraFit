/*
 * <one line to give the program's name and a brief idea of what it does.>
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

#include <charts.h>

#include "src/core/AbstractModel.h"
#include "src/core/toolset.h"

#include "src/ui/guitools/chartwrapper.h"
#include "src/ui/guitools/instance.h"
#include "src/ui/guitools/waiter.h"

#include "src/ui/widgets/results/mcresultswidget.h"
#include "src/ui/widgets/statisticwidget.h"

#include <QtCore/QPointer>
#include <QtCore/QThreadPool>

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QWidget>

#include "scatterwidget.h"

CollectThread::CollectThread(int start, int end, int var_1, int var_2, const QList<QJsonObject>& models, bool valid, bool converged)
    : m_start(start)
    , m_end(end)
    , m_var_1(var_1)
    , m_var_2(var_2)
    , m_models(models)
    , m_valid(valid)
    , m_converged(converged)
{
    setAutoDelete(false);
}

void CollectThread::setModel(QSharedPointer<AbstractModel> model)
{
    m_model = model->Clone(false);
}

void CollectThread::run()
{

    for (int i = m_start; i < m_end; ++i) {

        QJsonObject model;
        //if(!m_models[i].contains("initial"))
        model = m_models[i]; //["data"].toObject();
        //else
        //    model = m_models[i];

        m_model.data()->ImportModel(model);
        //m_model.data()->Calculate();
        //        qDebug() << model;

        if (m_converged && !model["converged"].toBool())
            continue;
        if (m_valid && !model["valid"].toBool())
            continue;

        m_x << m_model.data()->AllParameter()[m_var_1];
        m_y << m_model.data()->AllParameter()[m_var_2];
        m_linked_models.insert(QPointF(m_model.data()->AllParameter()[m_var_1], m_model.data()->AllParameter()[m_var_2]), i);
    }
}

ScatterWidget::ScatterWidget()

{
}

void ScatterWidget::setData(const QList<QJsonObject>& models, const QSharedPointer<AbstractModel> model)
{
    m_models = models;
    m_model = model->Clone(false);
    setUi();
}

void ScatterWidget::setUi()
{
    QGridLayout* layout = new QGridLayout;
    view = new ListChart;
    connect(view, &ListChart::LastDirChanged, this, [](const QString& str) {
        setLastDir(str);
    });
    view->setName("scatterwidget");
    connect(Instance::GlobalInstance(), &Instance::ConfigurationChanged, view, &ListChart::ApplyConfigurationChange);

    m_xy_series = new QtCharts::QScatterSeries;
    m_xy_series->setBorderColor(m_xy_series->color());
    QSplitter* splitter = new QSplitter(Qt::Vertical);
    splitter->addWidget(view);
    splitter->addWidget(VariWidget());
    layout->addWidget(splitter, 0, 0);

    setLayout(layout);
}

QWidget* ScatterWidget::VariWidget()
{
    QWidget* widget = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout;

    int index = 0;
    QHBoxLayout* hlayout = new QHBoxLayout;
    for (int i = 0; i < m_model.data()->GlobalParameterSize(); ++i) {
        QCheckBox* box = new QCheckBox;
        connect(box, &QCheckBox::stateChanged, box, [index, box, this](int state) {
            CheckBox(index, state);
        });

        connect(this, &ScatterWidget::Checked, box, [index, box, this](int var_1, int var_2) {
            if (var_1 == -1 || var_2 == -1)
                box->setEnabled(true);
            else
                box->setEnabled(var_1 == index || var_2 == index);
        });

        connect(this, &ScatterWidget::CheckParameterBox, box, [index, box, this](int parameter) {
            if (parameter == index)
                box->setChecked(true);
        });

        QLabel* label = new QLabel("<html>" + m_model.data()->GlobalParameterName(i) + "</html>");
        m_names << m_model.data()->GlobalParameterName(i);
        hlayout->addWidget(box);
        hlayout->addWidget(label, Qt::AlignLeft);

        connect(this, &ScatterWidget::HideBox, box, [index, box, label](int parameter) {
            if (parameter == index) {
                box->setHidden(true);
                label->setHidden(true);
            }
        });
        index++;
    }
    layout->addLayout(hlayout);

    for (int i = 0; i < m_model.data()->SeriesCount(); ++i) {
        QHBoxLayout* hlayout = new QHBoxLayout;
        for (int j = 0; j < m_model.data()->LocalParameterSize(); ++j) {
            QCheckBox* box = new QCheckBox;
            connect(box, &QCheckBox::stateChanged, box, [index, box, this](int state) {
                CheckBox(index, state);
            });

            connect(this, &ScatterWidget::Checked, box, [index, box, this](int var_1, int var_2) {
                if (var_1 == -1 || var_2 == -1)
                    box->setEnabled(true);
                else
                    box->setEnabled(var_1 == index || var_2 == index);
            });

            connect(this, &ScatterWidget::CheckParameterBox, box, [index, box, this](int parameter) {
                if (parameter == index)
                    box->setChecked(true);
            });

            QLabel* label = new QLabel("<html>" + QString::number(i + 1) + " - " + (m_model.data()->LocalParameterName(j)) + "</html>");
            m_names << QString::number(i + 1) + " - " + (m_model.data()->LocalParameterName(j));
            hlayout->addWidget(box);
            hlayout->addWidget(label, Qt::AlignLeft);

            connect(this, &ScatterWidget::HideBox, box, [index, box, label](int parameter) {
                if (parameter == index) {
                    box->setHidden(true);
                    label->setHidden(true);
                }
            });

            index++;
        }
        layout->addLayout(hlayout);
    }
    widget->setLayout(layout);
    return widget;
}

void ScatterWidget::CheckBox(int variable, int state)
{
    /* delete assigment */
    if (m_var_1 == variable && !state) {
        m_var_1 = -1;
        emit Checked(m_var_1, m_var_2);
        return;
    }

    if (m_var_2 == variable && !state) {
        m_var_2 = -1;
        emit Checked(m_var_1, m_var_2);
        return;
    }

    if (state == 2 && m_var_1 == -1)
        m_var_1 = variable;
    else if (state == 2 && m_var_2 == -1)
        m_var_2 = variable;

    emit Checked(m_var_1, m_var_2);
    MakePlot(m_var_1, m_var_2);
}

void ScatterWidget::Update()
{
    MakePlot(m_var_1, m_var_2);
}

void ScatterWidget::MakePlot(int var_1, int var_2)
{
    if (var_1 == -1 || var_2 == -1)
        return;

    m_linked_models_vector.clear();
    m_var_1 = var_1;
    m_var_2 = var_2;
    Waiter wait;
    view->Clear();
    view->Chart()->setAutoScaleStrategy(AutoScaleStrategy::QtNiceNumbers);

    QColor color = ChartWrapper::ColorCode(m_model.data()->Color(0)).lighter(50);
    m_xy_series = new QtCharts::QScatterSeries;

    QVector<CollectThread*> threads;
    int thread_count = qApp->instance()->property("threads").toInt();
    int step = m_models.size() / thread_count;
    QThreadPool* threadpool = QThreadPool::globalInstance();
    threadpool->setMaxThreadCount(thread_count);
    m_linked_models_vector.clear();
    int start = 0;
    int end = 0;
    for (int i = 0; i < thread_count; ++i) {
        start = end;
        if (i < thread_count - 1)
            end += step + 1;
        else
            end = m_models.size();
        CollectThread* thread = new CollectThread(start, end, m_var_1, m_var_2, m_models, m_valid, m_converged);
        thread->setModel(m_model);
        threadpool->start(thread);
        threads << thread;
    }
    while (threadpool->activeThreadCount())
        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

    QVector<qreal> x, y;
    for (int i = 0; i < thread_count; ++i) {
        x << threads[i]->X();
        y << threads[i]->Y();
        m_linked_models_vector << threads[i]->LinkedModels();
        delete threads[i];
    }

    if (x.size() > 1e4)
        m_xy_series->setUseOpenGL(true);
    for (int j = 0; j < x.size(); ++j)
        m_xy_series->append(QPointF(x[j], y[j]));

    PeakPick::LinearRegression regression = PeakPick::LeastSquares(ToolSet::QVector2Vector(x), ToolSet::QVector2Vector(y));

    connect(m_xy_series, &QtCharts::QXYSeries::clicked, this, &ScatterWidget::PointClicked);

    m_xy_series->setMarkerSize(7);
    m_xy_series->setName(m_names[var_1] + " vs. " + m_names[var_2]);
    view->addSeries(m_xy_series, 0, color, m_names[var_1] + " vs. " + m_names[var_2]);
    view->setXAxis(m_names[var_1]);
    view->setYAxis(m_names[var_2]);
    view->setTitle(QString("Scatter Plot %1 vs %2 (R%3 = %4)").arg(m_names[var_2]).arg(m_names[var_1]).arg(Unicode_Sup_2).arg(regression.R));
    m_xy_series->setColor(color);
    m_xy_series->setBorderColor(m_xy_series->color());
}

void ScatterWidget::PointClicked(const QPointF& point)
{
    QList<int> values;
    for (const auto item : qAsConst(m_linked_models_vector))
        values = item.values(point);
    if (values.isEmpty())
        return;

    emit ModelClicked(values.first());
}
