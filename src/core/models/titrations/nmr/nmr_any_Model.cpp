/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2022 Conrad Hübler <Conrad.Huebler@gmx.net>
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
#include "src/core/concentrationalpolynomial.h"
#include "src/core/equil.h"
#include "src/core/libmath.h"
#include "src/core/toolset.h"

#include "src/core/models/models.h"

#include <QDebug>
#include <QtMath>

#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QJsonObject>

#include "nmr_any_Model.h"

nmr_any_Model::nmr_any_Model(DataClass* data)
    : AbstractNMRModel(data)
{
    m_pre_input = { MaxA_Json, MaxB_Json };
    m_complete = false;
}

nmr_any_Model::nmr_any_Model(AbstractNMRModel* data)
    : AbstractNMRModel(data)
{
    DefineModel(m_model_definition);
    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
    DeclareOptions();
}

nmr_any_Model::~nmr_any_Model()
{
}

bool nmr_any_Model::DefineModel(const QJsonObject& model)
{
    m_model_definition = model;

    QJsonObject parse = model;
    if (parse.contains("ModelDefinition"))
        parse = model["ModelDefinition"].toObject();

    m_maxA = parse["MaxA"].toInt();
    m_maxB = parse["MaxB"].toInt();

    for (int i = 1; i <= m_maxA; ++i) {
        QString name_i = QString::number(i);
        QString name_i_short = QString::number(i);
        if (i == 1)
            name_i_short.clear();
        name_i = ToolSet::UnicodeLowerInteger(name_i);
        name_i_short = ToolSet::UnicodeLowerInteger(name_i_short);

        for (int j = 1; j <= m_maxB; ++j) {
            QString name_j = QString::number(j);
            QString name_j_short = QString::number(j);

            if (j == 1)
                name_j_short.clear();
            name_j = ToolSet::UnicodeLowerInteger(name_j);
            name_j_short = ToolSet::UnicodeLowerInteger(name_j_short);

            m_global_names << QString("lg %1%2%3").arg(Unicode_beta).arg(name_i).arg(name_j);
            m_species_names << QString("A%1B%2").arg(name_i_short).arg(name_j_short);
        }
    }

    m_global_parametersize = m_maxA * m_maxB;
    PrepareParameter(GlobalParameterSize(), LocalParameterSize());
    DeclareOptions();
    OptimizeParameters_Private();
    m_complete = true;
    return true;
}

void nmr_any_Model::InitialGuess_Private()
{
    LocalTable()->setColumn(DependentModel()->firstRow(), 0);
    // LocalTable()->setColumn(DependentModel()->lastRow(), 1);

    double guess_K = 4; // GuessK(0);
    (*GlobalTable())[0] = guess_K;
    for (int a = 1; a <= m_maxA; ++a)
        for (int b = 1; b <= m_maxB; ++b) {
            (*GlobalTable())[(Index(a, b))] = guess_K;
            // qDebug() << a << b << a*b << GlobalEnabled(a*b);
            LocalTable()->setColumn(DependentModel()->lastRow(), 1 + Index(a, b));
            // addLocalParameter(1+Index(a,b));
        }
    Calculate();
}

void nmr_any_Model::OptimizeParameters_Private()
{
    QString host = getOption(Host);
    for (int a = 1; a <= m_maxA; ++a)
        for (int b = 1; b <= m_maxB; ++b) {
            addGlobalParameter(Index(a, b));
            // qDebug() << a << b << a*b << GlobalEnabled(a*b);
            addLocalParameter(1 + Index(a, b));
        }
    if (host == "no")
        addLocalParameter(0);
}

void nmr_any_Model::CalculateVariables()
{
    ConcentrationalPolynomial poly;
    Vector constants(GlobalParameterSize());
    for (int i = 0; i < GlobalParameterSize(); ++i) {
        constants(i) = pow(10, GlobalParameter(i));
    }
    poly.setStabilityConstants(constants);
    poly.setStoichiometry(m_maxA, m_maxB);
    qreal value = 0;
    for (int i = 0; i < DataPoints(); ++i) {
        qreal host_0 = InitialHostConcentration(i);
        qreal guest_0 = InitialGuestConcentration(i);
        qreal host = ItoI::HostConcentration(host_0, guest_0, GlobalParameter(0));
        poly.setInitialConcentrations(host_0, guest_0);
        auto result = poly.solver();
        host = result(0);
        double guest = result(1);
        // qreal complex = host * guest * pow(10, GlobalParameter(0)); // host_0 - host;
        Vector vector(m_species_names.size() + 3);
        vector(0) = i + 1;
        vector(1) = host;
        vector(2) = guest;
        /*
        int index = 3;
        for (int a = 1; a <= m_maxA; ++a) {
            double powA = a*pow(host, a);
            for (int b = 1; b <= m_maxB; b++) {
                double beta = constants(poly.Index(a, b));
                const double c = (beta * pow(guest, b)*powA);
                vector(index++) = c;
            }
        }
        */
        for (int j = 0; j < SeriesCount(); ++j) {
            value = host / host_0 * LocalTable()->data(j, 0);
            for (int a = 1; a <= m_maxA; ++a) {
                double powA = a * pow(host, a);
                for (int b = 1; b <= m_maxB; b++) {
                    double beta = constants(poly.Index(a, b));
                    const double c = (beta * pow(guest, b) * powA);
                    vector(poly.Index(a, b) + 3) = c;
                    // value += (a * beta * pow(guest, b)*pow(host, a))/host_0*LocalTable()->data(j, a);
                    value += a * c / host_0 * LocalTable()->data(j, a);
                }
            }

            //+ complex / host_0 * LocalTable()->data(j, 1);
            SetValue(i, j, value);
        }
        if (!m_fast)
            SetConcentration(i, vector);
    }
}
/*
QVector<qreal> nmr_any_Model::DeCompose(int datapoint, int series) const
{
    QString method = getOption(Method);

    QVector<qreal> vector;
    qreal host_0 = InitialHostConcentration(datapoint);

    Vector concentration = getConcentration(datapoint);

    qreal host = concentration(1);

    qreal complex = concentration(3);
    ;

    vector << host / host_0 * LocalTable()->data(series, 0);
    vector << complex / host_0 * LocalTable()->data(series, 1);

    return vector;
}
*/
QSharedPointer<AbstractModel> nmr_any_Model::Clone(bool statistics)
{
    QSharedPointer<AbstractModel> model = QSharedPointer<nmr_any_Model>(new nmr_any_Model(this), &QObject::deleteLater);
    model.data()->ImportModel(ExportModel(statistics));
    model.data()->setActiveSignals(ActiveSignals());
    model.data()->setLockedParameter(LockedParameters());
    model.data()->setOptimizerConfig(getOptimizerConfig());
    return model;
}

QString nmr_any_Model::ModelInfo() const
{
    QString result = AbstractNMRModel::ModelInfo();
    result += BC50::ItoI::Format_BC50(GlobalParameter(0));

    return result;
}

QString nmr_any_Model::AdditionalOutput() const
{
    QString result;
    return result;

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
        // std::cout << host << " "
        //          << " " << guest_0 - complex << " " << complex << std::endl;
        // std::cout << integral.transpose() << std::endl;
    }
    integral(0) /= end;
    integral(1) /= end;
    integral(2) /= end;
    std::cout << integral.transpose() << std::endl;
}

QString nmr_any_Model::ParameterComment(int parameter) const
{
    Q_UNUSED(parameter)
    return QString("Reaction: A + B &#8652; AB");
}

QString nmr_any_Model::AnalyseMonteCarlo(const QJsonObject& object, bool forceAll) const
{

    QString result = AbstractNMRModel::AnalyseMonteCarlo(object, forceAll);

    if (!forceAll)
        return result;

    QString bc = Statistic::MonteCarlo2BC50_1(GlobalParameter(0), object);
    return bc + result;
}

QString nmr_any_Model::AnalyseGridSearch(const QJsonObject& object, bool forceAll) const
{

    QString result = AbstractNMRModel::AnalyseGridSearch(object, forceAll);

    if (!forceAll)
        return result;

    QString bc = Statistic::GridSearch2BC50_1(GlobalParameter(0), object);
    return bc + result;
}

#include "nmr_any_Model.moc"
