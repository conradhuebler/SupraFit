/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2017 - 2019 Conrad Hübler <Conrad.Huebler@gmx.net>
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
#include "src/core/dataclass.h"
#include "src/core/toolset.h"

#include <QtCharts/QAbstractSeries>
#include <QtCharts/QBoxPlotSeries>

#include <QtGui/QStandardItemModel>

#include "chartwrapper.h"


ChartWrapper::ChartWrapper(QObject* parent)
    : QObject(parent)
    , m_blocked(false)
    , m_transformed(false)
{
}

ChartWrapper::~ChartWrapper()
{
    for (int i = 0; i < m_stored_series.size(); ++i) {
        m_stored_series[i]->clear();
        delete m_stored_series[i];
    }

    m_stored_data.clear();
    m_stored_model.clear();
    m_working.clear();
#ifdef _DEBUG
    qDebug() << "Deleting chartwrapper";
#endif
}


void ChartWrapper::setData(QSharedPointer<DataClass> model)
{
    m_stored_data = model;
    m_working = m_stored_data;
    // can we make this more compact ?
    if (qobject_cast<AbstractModel*>(m_stored_data))
        connect(qobject_cast<AbstractModel*>(m_stored_data.data()), &AbstractModel::Recalculated, this, &ChartWrapper::UpdateModel);
    else if (qobject_cast<DataClass*>(m_stored_data))
        connect(m_stored_data.data(), &DataClass::Update, this, &ChartWrapper::UpdateModel);

    InitaliseSeries();
    UpdateModel();
}

void ChartWrapper::addWrapper(const QWeakPointer<ChartWrapper>& wrapper)
{
    if (m_stored_wrapper.contains(wrapper))
        return;

    m_stored_wrapper << wrapper;

    for (int i = 0; i < wrapper.data()->SeriesSize(); ++i) {
        QPointer<ScatterSeries> series = new ScatterSeries;
        for (const QPointF& point : wrapper.data()->Series(i)->points())
            series->append(point);

        series->setMarkerSize(qobject_cast<ScatterSeries*>(wrapper.data()->Series(i))->markerSize() * 0.75);
        series->setMarkerShape(qobject_cast<ScatterSeries*>(wrapper.data()->Series(i))->markerShape());
        //series->setColor(qobject_cast<ScatterSeries*>(wrapper.data()->Series(i))->color());
        //series->setBorderColor(qobject_cast<ScatterSeries*>(wrapper.data()->Series(i))->borderColor());
        //series->setBrush(qobject_cast<ScatterSeries*>(wrapper.data()->Series(i))->brush());

        connect(wrapper.data(), &ChartWrapper::ModelChanged, wrapper.data()->Series(i), [series, wrapper, i]() {
            if (!wrapper.data() || !series) {
                qDebug() << "series already left the building";
                return;
            }

            series->clear();
            for (const QPointF& point : wrapper.data()->Series(i)->points())
                series->append(point);

            // series->setMarkerSize(qobject_cast<ScatterSeries*>(wrapper.data()->Series(i))->markerSize()*0.75);
            // series->setMarkerShape(qobject_cast<ScatterSeries*>(wrapper.data()->Series(i))->markerShape());
            //series->setColor(qobject_cast<ScatterSeries*>(wrapper.data()->Series(i))->color());
            //series->setBrush(qobject_cast<ScatterSeries*>(wrapper.data()->Series(i))->brush());
        });

        m_stored_series << series.data();
        emit SeriesAdded(m_stored_series.size() - 1);
    }
}

void ChartWrapper::InitaliseSeries()
{
    if (!m_working) {
        m_working = m_stored_data;
        m_transformed = false;
    }

    if (m_stored_series.isEmpty()) {

        int serie = m_working.data()->SeriesCount();

        for (int j = 0; j < serie; ++j) {
            QPointer<QtCharts::QXYSeries> series;
            if (qobject_cast<AbstractModel*>(m_working.data()))
                series = new LineSeries;
            else
                series = new ScatterSeries;
            m_stored_series << series;
        }
    }
}

void ChartWrapper::UpdateModel()
{
    if (!m_working) {
        m_working = m_stored_data;
        m_transformed = false;
    }

    MakeSeries();
    emit ModelChanged();
}

void ChartWrapper::MakeSeries()
{
    if (!m_table)
        return;
    for (int j = 0; j < m_stored_series.size(); ++j)
        m_stored_series[j]->clear();

    int rows = m_working.data()->DataPoints();
    int cols = m_working.data()->SeriesCount();

    for (int i = 0; i < rows; ++i) {
        double x = m_working.data()->PrintOutIndependent(i);
        for (int j = 0; j < cols; ++j) {
            if (m_working.data()->DependentModel()->isChecked(j, i)) {
                if (j >= m_stored_series.size())
                    continue;
                m_stored_series[j]->append(x, m_table->data(j, i));
            }
        }
    }
}

QList<QPointer<QtCharts::QScatterSeries>> ChartWrapper::CloneSeries() const
{
    QList<QPointer<QtCharts::QScatterSeries>> series;
    for (int i = 0; i < m_stored_series.size(); ++i) {
        if (!m_stored_series[i]->isVisible())
            continue;
        QPointer<QtCharts::QScatterSeries> serie = new QtCharts::QScatterSeries();
        serie->append(m_stored_series[i]->points());
        serie->setColor(m_stored_series[i]->color());
        serie->setName(m_stored_series[i]->name());
        connect(m_stored_series[i].data(), &QtCharts::QAbstractSeries::nameChanged, serie.data(), [serie, i, this]() {

            if (serie && m_stored_series[i])
                serie->setName(m_stored_series[i]->name());

        });
        series << serie;
    }
    return series;
}

QColor ChartWrapper::color(int i) const
{
    if (m_stored_series.size() <= i)
        return ColorCode(i);
    else {
        QPointer<QtCharts::QXYSeries> series = m_stored_series[i];
        if (!series)
            return ColorCode(i);
        return m_stored_series[i]->color();
    }
}

QString ChartWrapper::ColorList() const
{
    QString list;
    for (int i = 0; i < m_stored_series.size(); ++i)
        list += color(i).name() + "|";
    list.chop(1);
    return list;
}

bool ChartWrapper::setColorList(const QString& str)
{
    QStringList colors = str.split("|");
    if (colors.size() != m_stored_series.size())
        return false;
    for (int i = 0; i < m_stored_series.size(); ++i)
        m_stored_series[i]->setColor(QColor(colors[i]));

    return true;
}

void ChartWrapper::TransformModel(QSharedPointer<DataClass> model)
{
    m_stored_model = model;
    /*
    if (!m_transformed)
        m_stored_model = model;
    else
        return;*/
    connect(m_stored_model.data(), SIGNAL(Recalculated()), this, SLOT(UpdateModel()));
    m_working = m_stored_model;
    m_transformed = true;
    MakeSeries();
    emit ModelTransformed();
}

QColor ChartWrapper::ColorCode(int i)
{
    switch (i % 9) {
    case 0:
        return QColor::fromRgbF(0.84, 0.93, 0.55, 1).darker(130);
    case 1:
        return QColor::fromRgbF(0.35, 0.47, 0.75, 1);
    case 2:
        return QColor::fromRgbF(1, 0.69, 0.37, 1);
    case 3:
        return QColor::fromRgbF(0.3, 0.73, 0.82, 1);
    case 4:
        return QColor::fromRgbF(0.95, 0.61, 0.7, 1);
    case 5:
        return QColor::fromRgbF(0.38, 0.78, 0.89, 1);
    case 6:
        return QColor::fromRgbF(0.37, 0.79, 0.51, 1);
    case 7:
        return QColor::fromRgbF(0.55, 0.82, 0.72, 1);
    case 8:
        return QColor::fromRgbF(0.67, 0.88, 0.38, 1);
    default:
        return Qt::darkGray;
    }
}

void ChartWrapper::SetBlocked(int blocked)
{
    m_blocked = !blocked;
}

void ChartWrapper::showSeries(int i)
{
    for (int j = 0; j < m_stored_series.size(); ++j) {
        if (i == -1 && !m_blocked) {
            m_stored_series[j]->setVisible(true);
            continue;
        }
        if (qobject_cast<ScatterSeries*>(m_stored_series[j]))
            m_stored_series[j]->setVisible(i == j);
        else if (i != -1 && m_stored_series[j]->isVisible())
            m_stored_series[j]->setVisible(i == j);
    }
    emit ShowSeries(i);
}
#include "chartwrapper.moc"
