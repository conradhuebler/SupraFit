/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016 - 2022 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/global.h"

#include <Eigen/Dense>

#include <QtCore/QDebug>
#include <QtCore/QJsonObject>
#include <QtCore/QObject>
#include <QtCore/QVector>
#include <QtCore/QtMath>

#include "src/core/models/AbstractModel.h"

typedef Eigen::VectorXd Vector;

struct MassResults {
    Vector MassBalance;
    Vector Components;
};

const QJsonObject MaxA_Json{
    { "name", "MaxA" },
    { "title", "Highest stoichiometry of A" },
    { "description", "Define the a, the highest stoichiometry in which A may appear" },
    { "value", 1 }, // default value
    { "type", 1 }, // 1 = int, 2 = double, 3 = string
    { "once", true }
};

const QJsonObject MaxB_Json{
    { "name", "MaxB" },
    { "title", "Highest stoichiometry of B" },
    { "description", "Define the b, the highest stoichiometry in which B may appear" },
    { "value", 1 }, // default value
    { "type", 1 }, // 1 = int, 2 = double, 3 = string
    { "once", true }
};

class AbstractTitrationModel : public AbstractModel {
    Q_OBJECT

public:
    enum {
        Temperature = 4
    };

    enum {
        Host = 1,
        Guest = 2
    };

    enum {
        HostGuest = 5,
        GuestHost = 6
    };

    enum {
        HostGuestAssignment = 1025
    };

    AbstractTitrationModel(DataClass* data);
    AbstractTitrationModel(AbstractTitrationModel* other);
    virtual ~AbstractTitrationModel() override;

    inline int Size() const override { return DataClass::Size(); }

    inline QVector<qreal*> getOptConstants() const { return m_opt_para; }

    virtual inline QString LocalParameterSuffix(int i = 0) const override
    {
        Q_UNUSED(i)
        return m_localParameterSuffix;
    }
    virtual inline QString LocalParameterDescription(int i = 0) const override
    {
        if (i == 0)
            return tr("%1 A").arg(Unicode_epsilion);
        else if (i == 1)
            return tr("%1 B").arg(Unicode_epsilion);
        else
            return tr("%1 %2").arg(Unicode_epsilion).arg(SpeciesName(i - 1));
    }
    inline virtual QString LocalParameterName(int i = 0) const override
    {
        if (i == 0)
            return tr("%1 A").arg(m_localParameterName);
        else if (i == 1)
            return tr("%1 B").arg(m_localParameterName);
        else
            return tr("%1 %2").arg(m_localParameterName).arg(SpeciesName(i - 1));
    }

    // virtual QString formatedGlobalParameter(qreal value, int globalParameter) const override;
    virtual QString SpeciesName(int i) const
    {
        Q_UNUSED(i)
        return QString();
    }
    /*
    virtual qreal BC50() const;
    virtual qreal BC50SF() const;*/
    virtual MassResults MassBalance(qreal A, qreal B);
    inline QPointer<DataTable> getConcentrations() const { return m_concentrations; }
    inline QPointer<DataTable> getConcentrations() { return m_concentrations; }

    inline Vector getConcentration(int row) const { return m_concentrations->Row(row); }

    /*! \brief we have two concentrations for all titration models, host and guest
     */
    virtual inline int InputParameterSize() const override { return 2; }
    virtual int LocalParameterSize(int series = 0) const override
    {
        Q_UNUSED(series)
        return GlobalParameterSize() + 2;
    }

    /*! \brief reimplmented from AbstractModel
     */
    virtual QString Model2Text_Private() const override;

    /*! \brief reimplementantion model dependented printout of the independant parameter
     */
    virtual qreal PrintOutIndependent(int i) const override;

    virtual QString ModelInfo() const override;

    virtual inline bool SupportSeries() const override { return true; }

    /*! \brief Define the x axis label for charts
     */
    virtual QString XLabel() const override { return m_plotMode; } // { return "G<sub>0</sub>/H<sub>0</sub>"; }

    /*! \brief Define the y axis for charts
     */
    virtual QString YLabel() const override { return m_ylabel; }

    virtual void DeclareOptions() override;

    virtual void EvaluateOptions() override;

    virtual inline int MaxParameter() override { return GlobalParameterSize() + LocalParameterSize() * SeriesCount(); }

    inline double getT() const { return m_T; }

    inline QString getPlotMode() const { return m_plotMode; }

    /*! \brief Calculate standard type statistics for stored statistic results */
    virtual QString AnalyseStatistic(bool forceAll = false) const override;

    /*! \brief Calculate standard type of monte carlo statistics */
    virtual QString AnalyseMonteCarlo(const QJsonObject& object, bool forceAll = false) const override;

    virtual QString AnalyseGridSearch(const QJsonObject& object, bool forceAll = false) const override;

    virtual QString ParameterComment(int parameter) const = 0;

    virtual QString AdditionalOutput() const override;

    //  virtual QVector<QJsonObject> PostGridSearch(const QList<QJsonObject> &models) const override;

public slots:
    virtual void UpdateParameter() override;

private:
    virtual void DeclareSystemParameter() override;

protected:
    /*
     * set the concentration of the @param int i datapoint to
     * @param const Vector& equilibrium, 
     * the vector holds the concentration of
     * each species in that model
     */
    void SetConcentration(int i, const Vector& equlibrium);

    qreal InitialHostConcentration(int i) const;
    qreal InitialGuestConcentration(int i) const;

    double m_T;

    QPair<bool, bool> getHostGuestPair() const;
    qreal GuessK(int index = 0);

    QString m_ylabel = "&delta; /ppm", m_plotMode, m_localParameterSuffix, m_localParameterDescription, m_localParameterName;

    QVector<QVector<qreal>> m_difference;

    QPointer<DataTable> m_concentrations;

    QStringList m_plotmode, m_HostAssignmentList;
    int m_HostAssignment = 0;
};
