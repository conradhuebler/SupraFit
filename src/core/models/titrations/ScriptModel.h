/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016  Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/global.h"

#ifdef experimental

#ifndef ScriptModel_H
#define ScriptModel_H

#include "src/core/AbstractModel.h"
#include "src/global.h"

#include <QDebug>
#include <QVector>
#include <QtCore/QJsonObject>
#include <QtCore/QMutex>
#include <QtCore/QObject>

#include <chaiscript/chaiscript.hpp>

#include "src/core/dataclass.h"

typedef Eigen::VectorXd Vector;

class ConcentrationSolver;
struct MassResults;

class ScriptModel : public AbstractTitrationModel {
    Q_OBJECT

public:
    ScriptModel(DataClass* data, const QJsonObject& json);
    virtual ~ScriptModel() override;

    virtual inline SupraFit::Model SFModel() const { return SupraFit::ScriptedModel; }

    virtual QVector<qreal> OptimizeParameters_Private(OptimizationType type);
    inline int ConstantSize() const { return Constants().size(); }

    virtual void InitialGuess();
    virtual QSharedPointer<AbstractTitrationModel> Clone() const;
    virtual bool SupportThreads() const { return false; }
    virtual qreal BC50();
    virtual MassResults MassBalance(qreal A, qreal B);
    virtual inline QStringList GlobalParameterNames() const override { return m_complex_names; }

private:
    QStringList m_constant_names;
    QVariantMap m_complex_map;
    QStringList m_component_list;
    QVariantHash m_complex_hashed;
    chaiscript::ChaiScript* chai;
    std::string m_signal_calculation;
    QJsonObject m_json;
    QVector<std::string> m_mass_balance;
    QMutex mutex;
    QList<QPointer<ConcentrationSolver>> m_solvers;
    /*
     * Reads the json file and sets the model up
     */
    void ParseJson();
    /*
     * Setup for the ChaiScript Engine
     */
    void InitializeCupofTea();
    /*
     * 
     */
    void CreateMassBalanceEquation(const QJsonObject& json);
    QList<qreal> m_constants_pow;

protected:
    QList<qreal> m_signals;
    virtual void CalculateVariables(const QList<qreal>& constants);
    virtual void DeclareParameter() override;
};

#endif // 1_1_Model
#endif
