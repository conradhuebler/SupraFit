/*
 * <one line to give the program's name and a brief idea of what it does.>
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

#include "src/core/equil.h"
#include "src/core/libmath.h"
#include "src/core/minimizer.h"
#include "src/core/models.h"

#include <QDebug>
#include <QtMath>

#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QJsonObject>

#include "meta_model.h"

MetaModel::MetaModel()
    : AbstractModel(new DataClass())
{
    PrepareParameter(0, 0);
}

MetaModel::~MetaModel()
{
    for (int i = 0; i < m_models.size(); ++i)
        m_models[i].clear();
}

void MetaModel::InitialGuess_Private()
{
    for (int i = 0; i < m_models.size(); ++i)
        m_models[i].data()->InitialGuess();

    Calculate();
}

QVector<qreal> MetaModel::OptimizeParameters_Private()
{
    QVector<qreal> parameter;
    for (int i = 0; i < m_models.size(); ++i)
        parameter << m_models[i].data()->OptimizeParameters();

    return parameter;
}

void MetaModel::addModel(const QPointer<AbstractModel> model)
{
    /* We will create a copy of that model
     */
    m_models << model->Clone();
    connect(model, &DataClass::NameChanged, m_models.last().data(), &DataClass::setProjectTitle);
    m_glob_param += model->GlobalParameterSize();
    m_inp_param += model->InputParameterSize();
    m_loc_param += model->LocalParameterSize();
    m_size += model->Size();
    m_indep_var += model->IndependentVariableSize();
    m_dep_var += model->DataPoints();
    m_series_count += model->SeriesCount();
    emit ModelAdded();
}

void MetaModel::CalculateVariables()
{
    for (int i = 0; i < m_models.size(); ++i)
        m_models[i].data()->Calculate();
}

QSharedPointer<AbstractModel> MetaModel::Clone()
{
    QSharedPointer<MetaModel> model = QSharedPointer<MetaModel>(new MetaModel, &QObject::deleteLater);

    for (const QSharedPointer<AbstractModel> m : Models())
        model.data()->addModel(m->Clone().data());

    return model;
}

#include "meta_model.moc"
