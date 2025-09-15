/*
 * Statistical Analysis Tools for SupraFit - Core analytical calculations
 * Copyright (C) 2018 - 2025 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <QtCore/QJsonObject>
#include <QtCore/QPair>
#include <QtCore/QString>
#include <QtCore/QVector>
#include <QtCore/QWeakPointer>

#include "src/core/models/AbstractModel.h"

namespace StatisticTool {

// TODO: Separate statistical calculations from string formatting
// Current functions return formatted strings - need JSON-based calculation functions
// for ML pipeline integration and universal access

/*
const QString Latex_Head = "\documentclass{standalone}"
        "\usepackage[utf8]{inputenc}"
        "\usepackage{tikz}"
        "\usepackage{pgfplots}"
        "\pgfplotsset{compat=1.10}"
        "\pgfplotsset{every axis/.append style={"
                            "label style={font=\sffamily},"
                            "tick label style={font=\sffamily}  "
                            "}}"
        "%1"
        "\begin{document}"
        "\begin{tikzpicture}[font=\sffamily\small]"
        "\begin{axis}[title={\parbox{6cm}{%2}},xmin = 0.5,xmax = %3, ymin= 0, ymax=0.25,xtick={%4},xticklabels={%5 }, ytick={%6},yticklabels={%7}, xlabel={Models}, ylabel={%8}, y label style={rotate=0,anchor=east,yshift=10pt}]"
        "%9"
        "\end{axis}"
        "%10"
        "\end{tikzpicture}"
        "\end{document}";

*/

// Legacy string-based analysis functions (for backward compatibility)
QString AnalyseReductionAnalysis(const QVector<QJsonObject> models, bool local, double cutoff);
QString CompareAIC(const QVector<QWeakPointer<AbstractModel>> models);
QString CompareCV(const QVector<QJsonObject> models, int cvtype, bool local, int cv_x);
QString CompareMC(const QVector<QJsonObject> models, bool local, int index);

// JSON-based statistical calculation functions (Claude Generated)
// These replace string-based functions for ML pipeline integration

/**
 * @brief Calculate AIC comparison metrics without string formatting
 * @param models Vector of fitted models to compare
 * @return QJsonObject with AIC values, rankings, and relative weights
 */
QJsonObject CalculateAICMetrics(const QVector<QWeakPointer<AbstractModel>>& models);

/**
 * @brief Calculate cross-validation statistics without string formatting  
 * @param models Vector of model results with CV data
 * @param cvtype Cross-validation type (1=leave-one-out, etc.)
 * @return QJsonObject with CV metrics, prediction errors, and validation scores
 */
QJsonObject CalculateCVMetrics(const QVector<QJsonObject>& models, int cvtype, int cv_x);

/**
 * @brief Calculate Monte Carlo parameter uncertainty without string formatting
 * @param models Vector of model results with MC data
 * @return QJsonObject with parameter uncertainties, confidence intervals, correlation matrix
 */
QJsonObject CalculateMCMetrics(const QVector<QJsonObject>& models, int index);

/**
 * @brief Calculate parameter reduction analysis without string formatting
 * @param models Vector of models for parameter significance analysis
 * @param cutoff Significance threshold for parameter inclusion
 * @return QJsonObject with parameter rankings, significance scores, reduced model suggestions
 */
QJsonObject CalculateReductionMetrics(const QVector<QJsonObject>& models, double cutoff);

/**
 * @brief Extract ML features from statistical analysis results
 * @param model Single model for feature extraction
 * @return QJsonObject with standardized ML features for training
 */
QJsonObject ExtractModelMLFeatures(QSharedPointer<AbstractModel> model);

/**
 * @brief Calculate Weakened Grid Search statistics without string formatting
 * @param models Vector of model results with grid search data
 * @return QJsonObject with grid exploration results and parameter sweeps
 */
QJsonObject CalculateWGSMetrics(const QVector<QJsonObject>& models);

/**
 * @brief Calculate Model Comparison statistics without string formatting
 * @param models Vector of model results with comparison data
 * @return QJsonObject with F-test results and model comparison metrics
 */
QJsonObject CalculateModelComparisonMetrics(const QVector<QJsonObject>& models);

/**
 * @brief Calculate Fast Confidence statistics without string formatting
 * @param models Vector of model results with confidence data
 * @return QJsonObject with simplified confidence intervals and estimates
 */
QJsonObject CalculateFastConfidenceMetrics(const QVector<QJsonObject>& models);

/**
 * @brief Calculate Global Search statistics without string formatting
 * @param models Vector of model results with global search data
 * @return QJsonObject with multi-start optimization results and convergence analysis
 */
QJsonObject CalculateGlobalSearchMetrics(const QVector<QJsonObject>& models);

/**
 * @brief Format statistical JSON data as human-readable string
 * @param statisticsJson JSON object with statistical results
 * @param analysisType Type of analysis ("AIC", "CV", "MC", "Reduction", "MLFeatures")
 * @return Formatted string for display/output
 */
QString FormatStatisticsString(const QJsonObject& statisticsJson, const QString& analysisType);
}
