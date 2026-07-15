/*
 * SupraFit ITC thermogram import orchestrator
 * Copyright (C) 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <QtCore/QList>

#include "src/core/models/datatable.h"
#include "src/core/thermogramhandler.h"
#include "src/core/toolset.h"

#include "itcprocessor.h"

ItcProcessor::ItcProcessor(QObject* parent)
    : QObject(parent)
{
    m_experiment = new ThermogramHandler();
    m_experiment->setParent(this);
    m_dilution = new ThermogramHandler();
    m_dilution->setParent(this);

    // Recompute the net heat whenever either thermogram re-integrates (interactive GUI edits or process()).
    connect(m_experiment, &ThermogramHandler::ThermogramChanged, this, &ItcProcessor::recomputeNetHeat);
    connect(m_dilution, &ThermogramHandler::ThermogramChanged, this, &ItcProcessor::recomputeNetHeat);
}

ItcProcessor::~ItcProcessor()
{
}

void ItcProcessor::setScalingFactor(qreal factor)
{
    m_experiment->setScalingFactor(factor);
    m_dilution->setScalingFactor(factor);
}

void ItcProcessor::process()
{
    m_experiment->Initialise();
    m_experiment->UpdatePeaks();
    m_experiment->AdjustIntegrationRange();
    m_experiment->IntegrateThermogram();

    if (m_use_dilution) {
        m_dilution->Initialise();
        m_dilution->UpdatePeaks();
        m_dilution->AdjustIntegrationRange();
        m_dilution->IntegrateThermogram();
    }

    recomputeNetHeat(); // ensure a consistent final state regardless of signal ordering during integration
}

void ItcProcessor::recomputeNetHeat()
{
    const QVector<qreal> exp = m_experiment->IntegralsScaled();
    QVector<qreal> net;
    if (m_use_dilution) {
        const QVector<qreal> dil = m_dilution->IntegralsScaled();
        net.reserve(exp.size());
        for (int i = 0; i < exp.size(); ++i)
            net << exp[i] - (i < dil.size() ? dil[i] : 0.0);
    } else {
        net = exp;
    }
    m_net_heat = net;
    emit resultChanged();
}

DataTable* ItcProcessor::resultTable() const
{
    DataTable* table = new DataTable;
    for (int i = 0; i < m_net_heat.size(); ++i) {
        QVector<qreal> row;
        row << (i < m_inject.size() ? m_inject[i] : 0.0) << m_net_heat[i];
        table->insertRow(row);
    }
    return table;
}

QJsonObject ItcProcessor::toJson() const
{
    QJsonObject root;

    QJsonObject experiment;
    experiment["fit"] = m_experiment->getThermogramParameter();
    root["experiment"] = experiment;

    if (m_use_dilution) {
        QJsonObject dilution;
        dilution["fit"] = m_dilution->getThermogramParameter();
        root["dilution"] = dilution;
    }

    // Canonical: the full per-injection volume vector under the "InjectVolume" key (matches
    // FileHandler::ReadITC's read key), instead of the legacy single scalar "injectvolume".
    root["InjectVolume"] = ToolSet::DoubleList2String(QList<qreal>(m_inject.begin(), m_inject.end()));
    return root;
}

void ItcProcessor::fromJson(const QJsonObject& obj)
{
    if (obj.contains("experiment"))
        m_experiment->setThermogramParameter(obj["experiment"].toObject()["fit"].toObject());

    m_use_dilution = obj.contains("dilution");
    if (m_use_dilution)
        m_dilution->setThermogramParameter(obj["dilution"].toObject()["fit"].toObject());

    if (obj.contains("InjectVolume") && obj["InjectVolume"].isString())
        m_inject = ToolSet::String2DoubleVec(obj["InjectVolume"].toString());
    // Legacy projects stored only a single "injectvolume" scalar; there the per-injection vector is
    // re-derived from the source ITC file by the caller, so nothing to restore here.
}
