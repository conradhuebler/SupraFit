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

#pragma once

#include "src/core/models/AbstractModel.h"
#include "src/global.h"

#include <QtCore/QMutex>
#include <QtCore/QObject>
#include <QtCore/QVector>

#include "src/core/models/dataclass.h"

class AbstractItcModel : public AbstractModel {
    Q_OBJECT

public:
    enum {
        CellVolume = 1,
        CellConcentration = 2,
        SyringeConcentration = 3,
        Temperature = 4,
        Reservoir = 5,
        InptUnit = 6
    };

    enum {
        Dilution = 2
    };

    AbstractItcModel(DataClass* data);
    AbstractItcModel(AbstractItcModel* data);

    virtual ~AbstractItcModel() override;

    inline virtual void ReleaseLocks() override { m_lock_concentrations = false; }

    /*
    virtual qreal BC50() const { return 0; }
    virtual inline qreal BC50SF() const { return BC50(); }
    */
    virtual qreal PrintOutIndependent(int i) const override;

    inline qreal InitialHostConcentration(int i) const { return m_c0->data(i, 1); }
    inline qreal InitialGuestConcentration(int i) const { return m_c0->data(i, 2); }
    QString Model2Text_Private() const override;

    virtual QString SpeciesName(int i) const
    {
        Q_UNUSED(i)
        return QString();
    }

    inline void setConcentrations(const QPointer<DataTable> table)
    {
        m_c0 = new DataTable(table);
        m_lock_concentrations = true;
    }
    inline QPointer<DataTable> ConcentrationTable() const { return m_c0; }

    /*! \brief Calculates the initial guess for the heat of formation
     * Defined for 1:1 system, but works for more complex systems as well
     * If not enough system parameter are defined, the function will fail
     * and be called after all parameter had been defined
     */
    qreal GuessdH();

    /*! \brief Calculates the initial guess for the fx factor
     * Defined for 1:1 system, but works for more complex systems as well
     * This correction or stoichiometry factor defines the number of binding sites
     * If not enough system parameter are defined, the function will fail
     * and be called after all parameter had been defined
     */
    qreal GuessFx();

    /*! \brief Calculates the initial guess for the K11
     * Defined for 1:1 system, but works for more complex systems as well
     *
     * If not enough system parameter are defined, the function will fail
     * and be called after all parameter had been defined
     */
    qreal GuessK(int index = 0);

    inline double getV() const { return getSystemParameter(CellVolume).Double(); }
    inline double getCellConcentration() const { return m_cell_concentration; }
    inline double getSyringeConcentration() const { return m_syringe_concentration; }
    inline double getT() const { return m_T; }
    inline QString getPlotMode() const { return m_plotMode; }

    virtual inline bool SupportSeries() const override { return false; }

    /*! \brief Define the x axis label for charts
     */
    virtual QString XLabel() const override { return m_plotMode; } // { return "G<sub>0</sub>/H<sub>0</sub>"; }

    /*! \brief Define the y axis for charts
     */
    virtual QString YLabel() const override { return "q"; }

    virtual QString ModelInfo() const override;

    virtual void UpdateOption(int index, const QString& str) override;

    virtual QString RandomInput(const QVector<double>& indep, const QVector<double>& dep) const override;

    inline virtual QString RandomExportSuffix() const override { return QString("Peak Files(*.dH)"); }

    /*! \brief Calculate standard type statistics for stored statistic results */
    virtual QString AnalyseStatistic(bool forceAll = false) const override;

    /*! \brief Calculate standard type of monte carlo statistics */
    virtual QString AnalyseMonteCarlo(const QJsonObject& object, bool forceAll = false) const override;

    virtual QString AnalyseGridSearch(const QJsonObject& object, bool forceAll = false) const override;

    virtual QString ParameterComment(int parameter) const = 0;

    // virtual QVector<QJsonObject> PostGridSearch(const QList<QJsonObject> &models) const override;

    inline virtual double ReductionCutOff() const override { return 1; }

public slots:
    virtual void UpdateParameter() override;

private:
    virtual void DeclareSystemParameter() override;
    bool m_lock_concentrations;
    QMutex m_lock;

private slots:
    void CalculateConcentrations();

protected:
    void SetConcentration(int i, const Vector& equilibrium);
    void virtual DeclareOptions() override;
    QPointer<DataTable> m_c0, m_concentrations;
    inline void Concentration() { CalculateConcentrations(); }
    double m_V, m_cell_concentration, m_syringe_concentration, m_T;
    bool m_reservior = true;
    QString m_plotMode;
    QStringList m_plotmode;
};
