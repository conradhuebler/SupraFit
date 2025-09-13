/*
 * NMR Model Selection Tutorial - Neural Network for Chemistry
 * Copyright (C) 2019 - 2025 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <QJsonObject>
#include <QString>
#include <QDebug>
#include <vector>
#include <memory>

#include "../../neural_network.h"
#include "../tutorial_manager.h"

/**
 * @brief Advanced tutorial demonstrating neural network model selection for NMR titrations
 * 
 * This tutorial showcases the practical application of neural networks in chemistry,
 * specifically for automatic NMR model selection based on experimental data features.
 * 
 * Educational Goals:
 * - Demonstrate feature extraction from chemical data
 * - Show classification vs regression in chemistry context
 * - Explain overfitting and model complexity in scientific applications
 * - Connect machine learning to actual chemical problems
 * 
 * Claude Generated - 2025
 */
class NMRModelSelectionTutorial {
public:
    /**
     * @brief Configuration for NMR tutorial complexity and interactivity
     */
    struct NMRTutorialConfig {
        bool interactive;
        bool show_feature_engineering;
        bool show_model_comparison;
        bool show_chemical_interpretation;
        TutorialLevel detail_level;
        
        NMRTutorialConfig() 
            : interactive(true)
            , show_feature_engineering(true)
            , show_model_comparison(true)
            , show_chemical_interpretation(true)
            , detail_level(TutorialLevel::Intermediate)
        {
        }
    };

    /**
     * @brief Sample NMR experiment data structure for training
     */
    struct NMRExperiment {
        QString experiment_id;
        Eigen::VectorXd features;          // Extracted features: chemical shifts, coupling constants, etc.
        QString optimal_model;             // Target: "1:1", "1:2", "2:1", "cooperative", etc.
        QString description;               // Human-readable explanation
        double confidence;                 // Experimental confidence in model assignment
    };

    explicit NMRModelSelectionTutorial(const NMRTutorialConfig& config = NMRTutorialConfig{});

    /**
     * @brief Main tutorial execution methods
     */
    void runFullTutorial();
    void runInteractive();
    bool validateModelSelection() const;

    /**
     * @brief Individual tutorial sections focused on chemistry
     */
    void explainFeatureExtraction();
    void demonstrateModelTypes();
    void buildFeatureClassifier();
    void testOnRealChemicalData();
    void explainChemicalRelevance();

    /**
     * @brief Create neural networks for different chemical problems
     */
    NeuralNetwork createNMRClassifier() const;
    NeuralNetwork createBindingAffinityPredictor() const;

    /**
     * @brief Chemical data management and validation
     */
    std::vector<NMRExperiment> createNMRTrainingData() const;
    bool testNMRNetwork(const NeuralNetwork& network, double tolerance = 0.2) const;

    /**
     * @brief Export tutorial results for SupraFit integration
     */
    QJsonObject exportNMRTutorialResults() const;

private:
    NMRTutorialConfig config_;
    std::vector<NMRExperiment> training_data_;
    
    /**
     * @brief Educational helper functions for chemistry context
     */
    void showChemicalStep(const QString& title, const QString& explanation);
    void showFeatureEngineering();
    void showModelInterpretation(const NeuralNetwork& network);
    void explainChemicalFeatures();
    void demonstrateOverfitting();
};