/*
 * Neural Network Training Tutorial for SupraFit
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
#include "../../loss_functions.h"
#include "../../optimizer.h"
#include "../tutorial_manager.h"

/**
 * @brief Comprehensive tutorial showing how to train neural networks from scratch
 * 
 * This tutorial demonstrates the complete training pipeline:
 * 1. Data preparation and feature engineering
 * 2. Network architecture design
 * 3. Loss function selection
 * 4. Optimizer configuration
 * 5. Training loop execution
 * 6. Performance evaluation and interpretation
 * 
 * Educational Goals:
 * - Show the complete ML workflow in chemistry context
 * - Explain training hyperparameters and their effects
 * - Demonstrate overfitting, underfitting, and regularization
 * - Connect training metrics to chemical understanding
 * 
 * Claude Generated - 2025
 */
class TrainingTutorial {
public:
    /**
     * @brief Configuration for training tutorial complexity
     */
    struct TrainingTutorialConfig {
        bool interactive;
        bool show_data_preparation;
        bool show_training_details;
        bool show_hyperparameter_effects;
        bool show_chemical_interpretation;
        TutorialLevel detail_level;
        
        TrainingTutorialConfig() 
            : interactive(true)
            , show_data_preparation(true)
            , show_training_details(true)
            , show_hyperparameter_effects(true)
            , show_chemical_interpretation(true)
            , detail_level(TutorialLevel::Intermediate)
        {
        }
    };

    explicit TrainingTutorial(const TrainingTutorialConfig& config = TrainingTutorialConfig{});

    /**
     * @brief Main tutorial execution methods
     */
    void runFullTutorial();
    void runInteractive();

    /**
     * @brief Individual tutorial sections
     */
    void explainTrainingProcess();
    void prepareTrainingData();
    void demonstrateHyperparameters();
    void trainXORFromScratch();
    void compareOptimizers();
    void analyzeLearningCurves();
    void explainChemistryApplications();

    /**
     * @brief Training examples for different problems
     */
    TrainingData createXORData() const;
    TrainingData createBindingData() const;  // Simulated binding affinity data
    TrainingData createClassificationData() const;  // NMR model classification data

    /**
     * @brief Training experiments with different configurations
     */
    NeuralNetwork::TrainingHistory trainWithConfig(const TrainingData& data, 
                                                   const TrainingConfig& config,
                                                   const QString& experiment_name);

    /**
     * @brief Export tutorial results
     */
    QJsonObject exportTrainingResults() const;

private:
    TrainingTutorialConfig config_;
    std::vector<std::pair<QString, NeuralNetwork::TrainingHistory>> training_experiments_;
    
    /**
     * @brief Helper functions for educational display
     */
    void showTrainingStep(const QString& title, const QString& explanation);
    void plotLearningCurves(const NeuralNetwork::TrainingHistory& history, const QString& title);
    void compareTrainingResults();
    void explainTrainingMetrics();
    void showDataDistribution(const TrainingData& data);
    void demonstrateOverfitting();
};