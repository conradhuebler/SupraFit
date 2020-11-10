/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2019 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QSharedPointer>

class AbstractModel;
class GlobalSearch;
class MonteCarloStatistics;
class ModelComparison;
class WeakenedGridSearch;
class ResampleAnalyse;

/* Standard Settings for Task are given here */

/* Model Comparison Settings */

const QJsonObject ModelComparisonConfigBlock{
    /* Set method */
    { "method", SupraFit::Method::ModelComparison }, // int, either SupraFit::Method::ModelComparison or SupraFit::Method::FastConfidence

    /* Maximal number of steps to be evaluated */
    { "MaxSteps", 1e4 }, // int

    /* Fast Confidence Maximal steps */
    { "MaxStepsFastConfidence", 1e3 }, // int

    /* Set scaling factor single step size */
    /* The factor determines the step length as follows:
     * delta = 10^(ceil(log10(parameter) + (-4) ))
     * this ensures correct scaling and always something like 10^(N)
     */
    { "FastConfidenceScaling", -4 }, // int

    /* Parameter threshold defined by f-Statistics */
    { "MaxParameter", 0 }, //double

    /* Confidence in % */
    { "confidence", 95 }, //double

    /* Statistical parameter to be analysed */
    { "ParameterIndex", 0 }, //int - this integer is the index for the vector inline QVector<qreal> StatisticVector() const { return QVector<qreal>() << SSE() << SEy() << ChiSquared() << sigma(); }
    // 0 - SSE; 1 - SEy, 2 - ChiSquared, 3 - sigma

    /* Corresponding f Value */
    { "f_value", 0 }, // double

    /* Threshold for convergency in SSE */
    { "ErrorConvergency", 1e-10 }, // double

    /* Box Scaling Factor - the default value, if none are set - there is no UI button for this */
    { "BoxScalingFactor", 1.5 }, // double

    /* Define the global and local parameter to be tested - this list should not be empty
     * when a model comparison search is performed, otherwise nothing happens at all, or it crashes ...*/
    { "GlobalParameterList", "" }, // strings, to be converted to QList<int>
    { "LocalParameterList", "" }, // strings, to be converted to QList<int>

    /* Define the box scaling factor of the global and local parameter to be tested - if this list is empty
        the scaling will automatically be set to 1.5 - and will most probably be not helpful */
    { "GlobalParameterScalingList", "" }, // strings, to be converted to QVector<double>
    { "LocalParameterScalingList", "" }, // strings, to be converted to QVector<double>

    /* Series in FastConfidence */
    { "IncludeSeries", true },

    /* Store intermediate results, may result in large json blocks */
    { "StoreRaw", true }, //bool

    /* Store as few data as possible */
    { "LightWeight", false }, //bool
};

/* Monte Carlo Settings */

const QJsonObject MonteCarloConfigBlock{
    /* Set method */
    { "method", SupraFit::Method::MonteCarlo }, // int

    /* Maximal number of Monte Carlo steps to be performed*/
    { "MaxSteps", 2e3 }, // int

    /* Variance to used */
    { "Variance", 1e-3 }, // double

    /* Confidence in % */
    { "confidence", 95 }, //double

    /* Source of variance */
    { "VarianceSource", 2 }, //int 1 = from "custom" above, 2 = "SEy", 3 = "sigma", 4 = "bootstrap"

    /* Use original data instead of the re-fitted data for monte carlo simulation  */
    { "OriginalData", false }, // bool

    /* Apply random numbers to the input vector/matrix
     * [0] - first row
       [1] - second row etc ... */
    { "IndependentRowVariance", "" }, // strings, to be converted to QVector<double>

    /* Set the number of bins for histogram calculation and entropy calclation */
    { "PlotBins", 30 }, //int, number of bins for histogram
    { "EntropyBins", 30 }, //int, number of bins for entropy

    /* Store intermediate results, may result in large json blocks */
    { "StoreRaw", true }, //bool

    /* Store as few data as possible */
    { "LightWeight", false }, //bool
};

/* Resample Methods Settings */
const QJsonObject ResampleConfigBlock{
    /* Set method */
    { "method", SupraFit::Method::CrossValidation }, // int either SupraFit::Method::CrossValidation or SupraFit::Method::ReductionAnalyse

    /* Leave X-Out Cross Validation */
    { "CXO", 1 }, // int 1 - Leave One Out; 2 - Leave Two Out; 3 - Leave Many Out - Please define via

    /* Leave X-Out */
    { "X", 3 }, // int, define how many have to be omitted

    /* Maximal number of steps to be evaluated, usefull for LXO CV, where many combination are possible */
    { "MaxSteps", 1e4 }, // int, 0 -> every combinition will be tested

    /* Algorithm Selection, either Precomputation, Automatic or Random */
    { "Algorithm", 2 }, // int 1 - Precomputation; 2 - Automatic; 3 - Random

    /* Set Runtype for Reduction Analysis */
    { "ReductionRuntype", 1 }, //int 1 - backward, 2 - forewards, 3 - both, backward and forewards

    /* Set the number of bins for histogram calculation and entropy calclation */
    { "PlotBins", 30 }, //int, number of bins for histogram
    { "EntropyBins", 30 }, //int, number of bins for entropy

    /* Store intermediate results, may result in large json blocks */
    { "StoreRaw", true }, //bool

    /* Store as few data as possible */
    { "LightWeight", false }, //bool

    /* Calculate Left-Out Points */
    { "LeftOutPoints", false } //bool

};

/* Grid Search Settings */
const QJsonObject GridSearchConfigBlock{
    /* Set method */
    { "method", SupraFit::Method::WeakenedGridSearch }, // int

    /* Maximal number of steps to be evaluated */
    { "MaxSteps", 1e3 }, // int

    /* Parameter threshold defined by f-Statistics */
    { "MaxParameter", 0 }, //double

    /* Confidence in % */
    { "confidence", 95 }, //double

    /* Corresponding f Value */
    { "f_value", 0 }, // double

    /* Statistical parameter to be analysed */
    { "ParameterIndex", 0 }, //int - this integer is the index for the vector inline QVector<qreal> StatisticVector() const { return QVector<qreal>() << SSE() << SEy() << ChiSquared() << sigma(); }
    // 0 - SSE; 1 - SEy, 2 - ChiSquared, 3 - sigma

    /* Threshold for convergency in SSE */
    { "ErrorConvergency", 1e-10 }, // double

    /* Maximal steps which are allowed to be above the SSE threshold, while continueing the evaluation */
    { "OvershotCounter", 5 }, // int

    /* Maximal number of steps, where the error is allowed to decrease, analyse not-converged systems, flat surfaces */
    { "ErrorDecreaseCounter", 50 }, // int

    /* Amount for all error changes below the threshold error_conv, while keeping the evaluation running */
    { "ErrorConvergencyCounter", 5 }, // int

    /* Set scaling factor single step size */
    /* The factor determines the step length as follows:
     * delta = 10^(ceil(log10(parameter) + (-4) ))
     * this ensures correct scaling and always something like 10^(N)
     */
    { "ScalingFactor", -4 }, // int

    /* Store intermediate results, may result in large json blocks */
    { "StoreRaw", false }, //bool

    /* Store as few data as possible */
    { "LightWeight", false }, //bool

    /* Define the global and local parameter to be tested - this list should not be empty
     * when a grid search is performed, otherwise nothing happens at all, or it crashes ...*/
    { "GlobalParameterList", "" }, // strings, to be converted to QList<int>
    { "LocalParameterList", "" } // strings, to be converted to QList<int>
};

class JobManager : public QObject {
    Q_OBJECT

public:
    JobManager(QObject* parent = 0);
    ~JobManager();

    inline void setModel(const QSharedPointer<AbstractModel>& model) { m_model = model; }
    void AddJob(const QJsonObject& job);

    void RunJobs();

    inline bool Working() const { return m_working; }
public slots:

private:
    QSharedPointer<AbstractModel> m_model;
    QList<QJsonObject> m_jobs;

    QJsonObject RunMonteCarlo(const QJsonObject& job);
    QJsonObject RunGridSearch(const QJsonObject& job);
    QJsonObject RunResample(const QJsonObject& job);
    QJsonObject RunModelComparison(const QJsonObject& job);
    QJsonObject RunGlobalSearch(const QJsonObject& job);

    QPointer<MonteCarloStatistics> m_montecarlo_handler;
    QPointer<WeakenedGridSearch> m_gridsearch_handler;
    QPointer<ModelComparison> m_modelcomparison_handler;
    QPointer<ResampleAnalyse> m_resample_handler;
    QPointer<GlobalSearch> m_globalsearch;

    bool m_working = false;
    bool m_interrupt = false;
    qint64 m_last_multicore = 0;

signals:
    void started();
    void finished(int current, int all, int time);
    void AllFinished();
    void incremented(int t);
    void prepare(int count);
    void ShowResult(SupraFit::Method type, int index);
    void Interrupt();
    void Message(const QString& str);
};
