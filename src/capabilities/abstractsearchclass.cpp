/*
 * <one line to give the library's name and an idea of what it does.>
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

#include "src/core/jsonhandler.h"

#include "abstractsearchclass.h"

AbstractSearchClass::AbstractSearchClass(QObject* parent)
    : QObject(parent)
    , m_interrupt(false)
{
    m_threadpool = QThreadPool::globalInstance();
    m_threadpool->setMaxThreadCount(qApp->instance()->property("threads").toInt());
}

AbstractSearchClass::~AbstractSearchClass()
{
    m_model.clear();
}

void AbstractSearchClass::Interrupt()
{
    m_interrupt = true;
    m_threadpool->clear();
}

void AbstractSearchClass::ExportResults(const QString& filename)
{
    QJsonObject toplevel;
    int i = 0;
    for (const QJsonObject& obj : qAsConst(m_models)) {
        QJsonObject constants = obj["data"].toObject()["constants"].toObject();
        QStringList keys = constants.keys();
        bool valid = true;
        for (const QString& str : qAsConst(keys)) {
            double var = constants[str].toString().toDouble();
            valid = valid && (var > 0);
        }
        toplevel["model_" + QString::number(i++)] = obj;
    }
    JsonHandler::WriteJsonFile(toplevel, filename);
}

QJsonObject AbstractSearchClass::Result() const
{
    QJsonObject result;
    for (int i = 0; i < m_results.size(); ++i)
        result[QString::number(i)] = m_results[i];
    QJsonObject controller = Controller();
    controller["title"] = m_model->Name();
    QJsonObject models;
    for (int i = 0; i < m_models.size(); ++i)
        models[QString::number(i)] = m_models[i];
    if (m_models.size()) {
        if (models["0"].toObject() == m_models[0])
            controller["raw"] = models;
    }
    result["controller"] = controller;

    return result;
}

QVector<Pair> AbstractSearchClass::DemandCalc()
{
    QMutexLocker lock(&mutex);

    if (m_batch.isEmpty())
        return QVector<Pair>();
    else
        return m_batch.dequeue();
}
