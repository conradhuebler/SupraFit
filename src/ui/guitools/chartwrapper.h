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

#pragma once

#include "src/core/toolset.h"

#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QPointer>

#include <QtCharts/QBoxPlotSeries>
#include <QtCharts/QLineSeries>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QVXYModelMapper>
#include <QtCharts/QXYSeries>

class DataClass;
class DataTable;
class QStandardItemModel;
class AbstractModel;

class LineSeries : public QtCharts::QLineSeries {
    Q_OBJECT

public:
    inline LineSeries()
        : m_dashdot(false)
        , m_size(2)
    {
    }
    inline ~LineSeries() {}

public slots:
    virtual void setColor(const QColor& color)
    {
        m_color = color;
        Update();
    }
    inline void setDashDotLine(bool dashdot)
    {
        m_dashdot = dashdot;
        Update();
    }
    inline void setSize(int size)
    {
        m_size = size;
        Update();
    }
    void ShowLine(int state);
    void ShowLine(bool state);
    virtual void setName(const QString& name);

private:
    inline void Update()
    {
        QPen pen = QtCharts::QLineSeries::pen();
        if (m_dashdot)
            pen.setStyle(Qt::DashDotLine);
        pen.setWidth(m_size);
        pen.setColor(m_color);
        setPen(pen);
    }
    bool m_dashdot;
    int m_size;
    QColor m_color;
};

class ScatterSeries : public QtCharts::QScatterSeries {
    Q_OBJECT

public:
    inline ScatterSeries() {}
    inline ~ScatterSeries() {}

public slots:
    virtual void setColor(const QColor& color);
    void ShowLine(int state);

signals:
    void NameChanged(const QString& str);
    void visibleChanged(int state);
};

class BoxPlotSeries : public QtCharts::QBoxPlotSeries {
    Q_OBJECT

public:
    BoxPlotSeries(const SupraFit::BoxWhisker& boxwhisker);
    inline QColor color() const { return brush().color(); }

public slots:
    void setColor(const QColor& color);
    virtual void setVisible(bool visible);

private:
    void LoadBoxWhisker();
    SupraFit::BoxWhisker m_boxwhisker;
    bool m_visible;
};

class ChartWrapper : public QObject {
    Q_OBJECT

public:
    ChartWrapper(bool flipable, QObject* parent = 0);
    ~ChartWrapper();
    void setData(QSharedPointer<DataClass> model);
    void addWrapper(const QWeakPointer<ChartWrapper>& wrapper);

    inline void setDataTable(const DataTable* table) { m_table = table; }
    inline int SeriesSize() const { return m_stored_series.size(); }
    inline QPointer<QtCharts::QXYSeries> Series(int i) { return m_stored_series[i]; }
    inline void setSeries(QPointer<QtCharts::QXYSeries> series, int i) { m_stored_series[i] = series; }
    QColor color(int i) const;
    void TransformModel(QSharedPointer<DataClass> model);
    QString ColorList() const;
    bool setColorList(const QString& str);

    QList<QPointer<QtCharts::QScatterSeries>> CloneSeries() const;
    QList<QWeakPointer<ChartWrapper>> m_stored_wrapper;

    static QColor ColorCode(int i);

public slots:
    void UpdateModel();
    void MakeSeries();
    void showSeries(int i);
    void SetBlocked(int blocked);

private:
    QPointer<const DataTable> m_table;
    QList<QPointer<QtCharts::QXYSeries>> m_stored_series;
    QSharedPointer<DataClass> m_model;
    bool m_blocked, m_transformed, m_flipable;
    void InitaliseSeries();

signals:
    void ModelChanged();
    void stopAnimiation();
    void restartAnimation();
    void ShowSeries(int i);
    void ModelTransformed();
    void SeriesAdded(int i);
};
