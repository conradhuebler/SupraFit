/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2017 - 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/core/AbstractModel.h"
#include "src/core/dataclass.h"
#include "src/core/toolset.h"

#include <QtCharts/QAbstractSeries>
#include <QtCharts/QBoxPlotSeries>

#include <QtGui/QStandardItemModel>

#include "chartwrapper.h"

void LineSeries::ShowLine(bool state)
{
    setVisible(state);
}

void LineSeries::ShowLine(int state)
{
    if (state == Qt::Unchecked)
        setVisible(false);
    else if (state == Qt::Checked)
        setVisible(true);
}

void LineSeries::setName(const QString& str)
{
    QtCharts::QLineSeries::setName(str);
}

void ScatterSeries::setColor(const QColor& color)
{
    QPen pen = QtCharts::QScatterSeries::pen();
    //      pen.setStyle(Qt::DashDotLine);
    pen.setWidth(2);
    //      pen.setColor(color);
    QScatterSeries::setColor(color);
    setPen(pen);
}

void ScatterSeries::ShowLine(int state)
{
    if (state == Qt::Unchecked)
        setVisible(false);
    else if (state == Qt::Checked)
        setVisible(true);
    emit visibleChanged(state);
}

BoxPlotSeries::BoxPlotSeries(const SupraFit::BoxWhisker& boxwhisker)
    : m_boxwhisker(boxwhisker)
{
    LoadBoxWhisker();
    m_visible = true;
}

void BoxPlotSeries::LoadBoxWhisker()
{
    QtCharts::QBoxSet* box = new QtCharts::QBoxSet;
    box->setValue(QtCharts::QBoxSet::LowerExtreme, m_boxwhisker.lower_whisker);
    box->setValue(QtCharts::QBoxSet::UpperExtreme, m_boxwhisker.upper_whisker);
    box->setValue(QtCharts::QBoxSet::Median, m_boxwhisker.median);
    box->setValue(QtCharts::QBoxSet::LowerQuartile, m_boxwhisker.lower_quantile);
    box->setValue(QtCharts::QBoxSet::UpperQuartile, m_boxwhisker.upper_quantile);
    append(box);
}

void BoxPlotSeries::setVisible(bool visible)
{
    if (m_visible == visible)
        return;
    if (visible)
        LoadBoxWhisker();
    else
        clear();
    m_visible = visible;
}

void BoxPlotSeries::setColor(const QColor& color)
{
    QBrush brush;
    brush.setColor(color);
    setBrush(brush);
}

ChartWrapper::ChartWrapper(bool flipable, QObject* parent)
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
    m_stored_wrapper << wrapper;

    for (int i = 0; i < wrapper.data()->SeriesSize(); ++i) {
        ScatterSeries* series = new ScatterSeries;
        for (const QPointF& point : wrapper.data()->Series(i)->points()) {
            series->append(point);
            series->setMarkerSize(qobject_cast<ScatterSeries*>(wrapper.data()->Series(i))->markerSize());
            series->setMarkerShape(qobject_cast<ScatterSeries*>(wrapper.data()->Series(i))->markerShape());
            series->setColor(qobject_cast<ScatterSeries*>(wrapper.data()->Series(i))->color());
            series->setBorderColor(qobject_cast<ScatterSeries*>(wrapper.data()->Series(i))->borderColor());
            series->setBrush(qobject_cast<ScatterSeries*>(wrapper.data()->Series(i))->brush());
        }

        connect(wrapper.data(), &ChartWrapper::ModelChanged, wrapper.data()->Series(i), [series, wrapper, i]() {
            series->clear();
            for (const QPointF& point : wrapper.data()->Series(i)->points())
                series->append(point);

            series->setMarkerSize(qobject_cast<ScatterSeries*>(wrapper.data()->Series(i))->markerSize());
            series->setMarkerShape(qobject_cast<ScatterSeries*>(wrapper.data()->Series(i))->markerShape());
            series->setColor(qobject_cast<ScatterSeries*>(wrapper.data()->Series(i))->color());
            series->setBrush(qobject_cast<ScatterSeries*>(wrapper.data()->Series(i))->brush());
        });

        m_stored_series << series;
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
        QtCharts::QScatterSeries* serie = new QtCharts::QScatterSeries();
        serie->append(m_stored_series[i]->points());
        serie->setColor(m_stored_series[i]->color());
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
    switch (i) {
    case 0:
        return Qt::darkCyan;
    case 1:
        return QColor(Qt::blue).lighter();
    case 2:
        return QColor(Qt::darkCyan).lighter();
    case 3:
        return QColor(Qt::darkGreen).lighter();
    case 4:
        return Qt::darkRed;
    case 5:
        return Qt::darkBlue;
    case 6:
        return Qt::darkGreen;
    case 7:
        return Qt::cyan;
    case 8:
        return Qt::red;
    case 9:
        return Qt::blue;
    case 10:
        return Qt::darkMagenta;
    case 11:
        return Qt::darkYellow;
    case 12:
        return Qt::gray;
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
