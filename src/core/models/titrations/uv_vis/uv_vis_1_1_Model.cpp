/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016 - 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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
#include "src/core/models/postprocess/statistic.h"

#include "src/core/bc50.h"
#include "src/core/equil.h"
#include "src/core/libmath.h"
#include "src/core/toolset.h"

#include "src/core/models/models.h"

#include <QDebug>
#include <QtMath>

#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QJsonObject>

#include "uv_vis_1_1_Model.h"

uv_vis_ItoI_Model::uv_vis_ItoI_Model(DataClass* data)
    : AbstractTitrationModel(data)
{
    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
    DeclareOptions();
}

uv_vis_ItoI_Model::uv_vis_ItoI_Model(AbstractTitrationModel* data)
    : AbstractTitrationModel(data)
{
    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
    DeclareOptions();
}

uv_vis_ItoI_Model::~uv_vis_ItoI_Model()
{
}

void uv_vis_ItoI_Model::InitialGuess_Private()
{
    qreal factor = 1;
    factor = 1 / InitialHostConcentration(0);

    LocalTable()->setColumn(DependentModel()->firstRow() * factor, 0);
    LocalTable()->setColumn(DependentModel()->lastRow() * factor, 2);

    (*GlobalTable())[0] = GuessK(0);

    Calculate();
}

void uv_vis_ItoI_Model::CollectOptimizationParameters_Private()
{
    QString host = getOption(Host);
    QString guest = getOption(Guest);

    addGlobalParameter(0);
    if (host == "no")
        addLocalParameter(0);

    if (guest == "no")
        addLocalParameter(1);

    addLocalParameter(2);
}

void uv_vis_ItoI_Model::FillDesign()
{
    if (m_design.rows() != DataPoints() || m_design.cols() != 3)
        m_design.resize(DataPoints(), 3);
    const auto hostguest = getHostGuestPair();
    const double hf = hostguest.first ? 1.0 : 0.0; // free host absorbs?
    const double gf = hostguest.second ? 1.0 : 0.0; // free guest absorbs?
    for (int i = 0; i < DataPoints(); ++i) {
        qreal host_0 = InitialHostConcentration(i);
        qreal guest_0 = InitialGuestConcentration(i);
        qreal host = ItoI::HostConcentration(host_0, guest_0, GlobalParameter(0));
        qreal complex = host_0 - host;
        qreal guest = guest_0 - complex;
        m_design(i, 0) = host * hf; // free-host extinction column (eps_A)
        m_design(i, 1) = guest * gf; // free-guest extinction column (eps_B)
        m_design(i, 2) = complex; // complex extinction column (eps_AB)
        if (!m_fast) {
            Vector vector(4);
            vector(0) = i + 1;
            vector(1) = host;
            vector(2) = guest_0 - complex;
            vector(3) = complex;
            SetConcentration(i, vector);
        }
    }
}

void uv_vis_ItoI_Model::CalculateVariables()
{
    FillDesign();
    // Beer-Lambert absorbance = sum of concentration*extinction over the absorbing species. Same math
    // as before, expressed through the shared design matrix (the host/guest columns already carry the
    // "silent component" 0/1 multipliers).
    for (int i = 0; i < DataPoints(); ++i)
        for (int j = 0; j < SeriesCount(); ++j)
            SetValue(i, j,
                m_design(i, 0) * LocalTable()->data(j, 0) + m_design(i, 1) * LocalTable()->data(j, 1) + m_design(i, 2) * LocalTable()->data(j, 2));
}

void uv_vis_ItoI_Model::ProjectLinearParameters()
{
    FillDesign();
    SolveLinearMasked(m_design);
}

QVector<qreal> uv_vis_ItoI_Model::DeCompose(int datapoint, int series) const
{
    QVector<qreal> vector;
    qreal host_0 = InitialHostConcentration(datapoint);

    Vector concentration = getConcentration(datapoint);

    qreal host = concentration(1);

    qreal complex = concentration(3);

    host_0 = 1;

    vector << host / host_0 * LocalTable()->data(series, 0);
    vector << complex / host_0 * LocalTable()->data(series, 1);

    return vector;
}

QSharedPointer<AbstractModel> uv_vis_ItoI_Model::Clone(bool statistics)
{
    QSharedPointer<AbstractModel> model = QSharedPointer<uv_vis_ItoI_Model>(new uv_vis_ItoI_Model(this), &QObject::deleteLater);
    finishClone(model, statistics);
    return model;
}

QString uv_vis_ItoI_Model::ModelInfo() const
{
    QString result = AbstractTitrationModel::ModelInfo();
    result += BC50::ItoI::Format_BC50(GlobalParameter(0));

    return result;
}

QString uv_vis_ItoI_Model::AdditionalOutput() const
{
    QString result;
    return result;

    // double max = 1e3;
    double delta = 1e-3;
    qreal host_0 = 1.0;
    qreal host = 0;
    qreal diff = host_0 - host;
    Vector integral(3);
    qreal end = delta;
    for (end = delta; diff > 1e-5; end += delta) {
        qreal guest_0 = end;
        host = ItoI::HostConcentration(host_0, guest_0, GlobalParameter(0));
        qreal complex = host_0 - host;

        integral(0) += host * delta;
        integral(1) += (guest_0 - complex) * delta;
        integral(2) += complex * delta;

        diff = host_0 - complex;
        // std::cout << end << " " << diff << " " << host << " " << " " << guest_0 - complex << " " << complex << std::endl;
        //std::cout << host << " "
        //          << " " << guest_0 - complex << " " << complex << std::endl;
        //std::cout << integral.transpose() << std::endl;
    }
    integral(0) /= end;
    integral(1) /= end;
    integral(2) /= end;
}

QString uv_vis_ItoI_Model::ParameterComment(int parameter) const
{
    Q_UNUSED(parameter)
    return QString("Reaction: A + B &#8652; AB");
}

QString uv_vis_ItoI_Model::AnalyseMonteCarlo(const QJsonObject& object, bool forceAll) const
{
    return prependBC50(AbstractTitrationModel::AnalyseMonteCarlo(object, forceAll), forceAll, Statistic::MonteCarlo2BC50_1(GlobalParameter(0), object));
}

QString uv_vis_ItoI_Model::AnalyseGridSearch(const QJsonObject& object, bool forceAll) const
{
    return prependBC50(AbstractTitrationModel::AnalyseGridSearch(object, forceAll), forceAll, Statistic::GridSearch2BC50_1(GlobalParameter(0), object));
}

#include "uv_vis_1_1_Model.moc"
