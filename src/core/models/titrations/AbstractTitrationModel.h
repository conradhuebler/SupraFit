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
#include "src/core/speciationengine.h"

typedef Eigen::VectorXd Vector;

struct MassResults {
    Vector MassBalance;
    Vector Components;
};

// Free-text reaction equations defining the N-component equilibrium system, one reaction per line in
// arrow syntax, e.g. "A + B <=> AB\n2 A <=> A2\nA + C <=> AC". Parsed by ReactionParser into the
// component/species stoichiometry driving the BFGS speciation solver. Required: an empty field leaves
// the model undefined (the legacy MaxA/MaxB/MaxSelfA/Species grid was removed). Claude Generated.
const QJsonObject Reactions_Json{
    { "name", "Reactions" },
    { "title", "Reaction equations" },
    { "description", "Reaction equations, one per line, e.g. 'A + B <=> AB'. Defines the N-component equilibrium system driving the speciation solver." },
    { "value", "" },
    { "type", 6 }, // 1 = int, 2 = double, 3 = string, 4 = script text, 5 = species editor, 6 = reaction editor
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

    /*! \brief > 0 if the last DefineModel() parsed a valid reaction system whose component count did
     * not match the number of independent concentration columns (the model then fell back to the
     * grid). The value is the number of components the reactions requested. Claude Generated. */
    inline int ReactionComponentMismatch() const { return m_reaction_component_mismatch; }

    /*! \brief number of free components (independent concentration columns). Two by default
     * (host + guest); an N-component reaction system raises it (see BuildSpeciationFromReactions).
     */
    virtual inline int InputParameterSize() const override { return m_component_count; }
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

    // Adds the Musketeer citation once speciation ran through the BFGS engine. Claude Generated.
    inline QStringList CitationKeys() const override
    {
        QStringList keys;
        if (m_uses_bfgs)
            keys << QStringLiteral("musketeer");
        return keys;
    }

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

    /*! \brief Total (analytical) concentration of free component @p component at data point @p i.
     * Generalises InitialHost/GuestConcentration to arbitrary components; component 0/1 are the
     * classic host/guest. Claude Generated. */
    qreal InitialConcentration(int i, int component) const;

    /*! \brief Parse the "Reactions" model-definition field into an N-component speciation system.
     * On success sets m_component_count / m_component_names, configures m_speciation, refreshes the
     * independent-table headers and returns true. Empty/invalid input leaves the 2-component grid
     * defaults untouched and returns false. Claude Generated. */
    bool BuildSpeciationFromReactions();

    /*! \brief Set the independent-table column headers from m_component_names. Claude Generated. */
    void UpdateComponentHeaders();

    /*! \brief Data-derived initial guess for the cumulative stability constant lg(beta) of species
     * @p speciesIndex. Scales the constant to the concentration range of the actual data via
     * lg(beta) ~ (order - 1) * (-lg c_ref), where the reaction order is the sum of the species'
     * stoichiometric coefficients and c_ref is the geometric-mean per-component maximum total. This
     * keeps higher-order complexes (e.g. AB2) from starting far too high, which otherwise sends the
     * optimiser into a flat runaway direction. Claude Generated. */
    double GuessLgBeta(int speciesIndex) const;

    int m_component_count = 2; ///< number of free components (2 = classic host/guest)
    QStringList m_component_names; ///< component symbols, e.g. ["A","B","C"] (empty => host/guest)
    SpeciationEngine m_speciation; ///< reaction system + BFGS solver (used by the *_any models)
    bool m_uses_bfgs = false; ///< true once speciation ran through the BFGS engine (citation hint)
    int m_reaction_component_mismatch = 0; ///< components requested by reactions if != data columns

    double m_T = 298; // K — default so getT() is never uninitialised (system parameter overrides)

    QPair<bool, bool> getHostGuestPair() const;
    qreal GuessK(int index = 0, double min = 1, double max = 5);

    QString m_ylabel = "&delta; /ppm", m_plotMode, m_localParameterSuffix, m_localParameterDescription, m_localParameterName;

    QVector<QVector<qreal>> m_difference;

    QPointer<DataTable> m_concentrations;

    QStringList m_plotmode, m_HostAssignmentList;
    int m_HostAssignment = 0;
};
