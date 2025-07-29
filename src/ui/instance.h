/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2018 - 2022 Conrad Hübler <Conrad.Huebler@gmx.net>
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
 */

#pragma once

#include <QtCore/QCoreApplication>
#include <QtCore/QJsonObject>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QUuid>
#include <QtCore/QVariant>

#include <charts.h>

template <class T>
class InstancePointer {
public:
    static T* asPtr(QVariant v)
    {
        return (T*)v.value<void*>();
    }
    static QVariant asQVariant(T* ptr)
    {
        return QVariant::fromValue((void*)ptr);
    }
};

class Instance : public QObject {
    Q_OBJECT

public:
    Instance()
    {
        QUuid uuid;
        m_uuid = uuid.createUuid().toString();
    }
    inline static void setInstance(QPointer<Instance> instance) { qApp->instance()->setProperty("InstanceHolder", InstancePointer<Instance>::asQVariant(instance)); }
    inline static Instance* GlobalInstance() { return InstancePointer<Instance>::asPtr(qApp->instance()->property("InstanceHolder")); }

    inline QString UUid() const { return m_uuid; }

    inline QJsonObject ChartConfig()
    {
        QJsonObject config = m_chartconfig;

        config["markerSize"] = qApp->instance()->property("markerSize").toDouble();
        config["lineWidth"] = qApp->instance()->property("lineWidth").toDouble();
        config["xSize"] = qApp->instance()->property("xSize").toDouble();
        config["ySize"] = qApp->instance()->property("ySize").toDouble();
        config["transparentImage"] = qApp->instance()->property("transparentChart").toBool();
        config["cropImage"] = qApp->instance()->property("cropedChart").toBool();
        config["noGrid"] = qApp->instance()->property("noGrid").toBool();
        config["emphasizeAxis"] = qApp->instance()->property("empAxis").toBool();
        return config;
    }

    inline void WriteChartConfig(const QJsonObject& config)
    {
        qApp->instance()->setProperty("markerSize", config["markerSize"].toDouble());
        qApp->instance()->setProperty("lineWidth", config["lineWidth"].toDouble());
        qApp->instance()->setProperty("xSize", config["xSize"].toDouble());
        qApp->instance()->setProperty("ySize", config["ySize"].toDouble());
        qApp->instance()->setProperty("transparentChart", config["transparentImage"].toDouble());
        qApp->instance()->setProperty("cropedChart", config["cropImage"].toDouble());
        qApp->instance()->setProperty("noGrid", config["noGrid"].toDouble());
        qApp->instance()->setProperty("empAxis", config["emphasizeAxis"].toDouble());
        m_chartconfig = config;
        //  config["lineWidth"] = qApp->instance()->property("lineWidth").toDouble();
        //  config["xSize"] = qApp->instance()->property("xSize").toDouble();
        //  config["ySize"] = qApp->instance()->property("ySize").toDouble();
        //  config["transparentImage"] = qApp->instance()->property("transparentChart").toBool();
        //  config["cropImage"] = qApp->instance()->property("cropedChart").toBool();
        //  config["noGrid"] = qApp->instance()->property("noGrid").toBool();
        //  config["emphasizeAxis"] = qApp->instance()->property("empAxis").toBool();
    }

    inline void updateChartConfig(const QJsonObject& config)
    {
        WriteChartConfig(config);
        emit ConfigurationUpdated();
    }

    inline void UpdateFontConfig(const QJsonObject& font)
    {
        m_fontconfig = font;
        emit FontConfigurationChanged();
    }
    inline QJsonObject FontConfig() const { return m_fontconfig; }

    void AddFontConfigFile(const QString& str, const QString& description, const QJsonObject& data)
    {
        qDebug() << str << description << data;
        emit FontConfigFileAdded(str, description, data);
    }
    void MakeChartConnections(QPointer<ListChart> chart)
    {
        connect(Instance::GlobalInstance(), &Instance::FontConfigurationChanged, chart, [chart]() {
            chart->updateChartConfig(Instance::GlobalInstance()->FontConfig(), false);
        });

        connect(Instance::GlobalInstance(), &Instance::ConfigurationChanged, chart, [chart]() {
            chart->updateChartConfig(Instance::GlobalInstance()->ChartConfig(), true);
        });
#pragma message ("restore the connection, once cutecharts supports it again")
        //connect(chart, &ListChart::configurationChanged, this, [chart]() {
          //  Instance::GlobalInstance()->UpdateFontConfig(chart->currentFontConfig());
        //});

        //connect(chart, &ListChart::exportSettingsFileAdded, this, &Instance::AddFontConfigFile);
        //connect(this, &Instance::FontConfigFileAdded, chart, &ListChart::addExportSetting);

        QMetaObject::Connection connection;
        connection = connect(chart, &ListChart::setUpFinished, this, [this, chart, connection]() {
            chart->setFontConfig(m_fontconfig);
            disconnect(connection);
        });
    }

    void MakeChartConnections(QPointer<ChartView> chart)
    {
        if (!m_fontconfig.isEmpty())
            chart->updateChartConfig(m_fontconfig, true);

        connect(Instance::GlobalInstance(), &Instance::FontConfigurationChanged, chart, [chart]() {
            chart->updateChartConfig(Instance::GlobalInstance()->FontConfig(), false);
        });

        connect(Instance::GlobalInstance(), &Instance::ConfigurationChanged, chart, [chart]() {
            chart->updateChartConfig(Instance::GlobalInstance()->ChartConfig(), true);
        });

        connect(chart, &ChartView::configurationChanged, this, [chart]() {
            Instance::GlobalInstance()->UpdateFontConfig(chart->currentFontConfig());
        });

        connect(chart, &ChartView::exportSettingsFileAdded, this, &Instance::AddFontConfigFile);
        connect(this, &Instance::FontConfigFileAdded, chart, &ChartView::addExportSetting);

        QMetaObject::Connection connection;
        connection = connect(chart, &ChartView::setUpFinished, this, [this, chart, connection]() {
            chart->setFontConfig(m_fontconfig);
            disconnect(connection);
        });
    }

private:
    QString m_uuid;
    QJsonObject m_chartconfig, m_fontconfig;

signals:
    void ConfigurationChanged();
    void ConfigurationUpdated();
    void FontConfigurationChanged();
    void FontConfigFileAdded(const QString& str, const QString& description, const QJsonObject& data);
};
