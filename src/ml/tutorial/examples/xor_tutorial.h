/*
 * XOR Tutorial - Interactive Neural Network Learning
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

#include <vector>
#include <QString>
#include <QJsonObject>
#include <Eigen/Dense>

#include "../../neural_network.h"

/**
 * @brief XOR Problem Tutorial - Neural Network Education
 * 
 * The XOR (exclusive OR) problem is the "Hello World" of neural networks.
 * It demonstrates why neural networks need hidden layers and non-linear
 * activation functions to solve non-linearly separable problems.
 * 
 * Educational Goals:
 * - Understand why single perceptrons fail on XOR
 * - See how hidden layers enable non-linear decision boundaries
 * - Learn about activation functions and their roles
 * - Practice with matrix operations in neural networks
 * 
 * XOR Truth Table:
 * Input A | Input B | Output
 * --------|---------|-------
 *    0    |    0    |   0
 *    0    |    1    |   1
 *    1    |    0    |   1
 *    1    |    1    |   0
 * 
 * Claude Generated - 2025
 */
class XORTutorial {
public:
    /**
     * @brief Training data sample
     */
    struct TrainingSample {
        Eigen::VectorXd input;
        double expected_output;
        QString description;
    };

    /**
     * @brief Tutorial configuration
     */
    struct TutorialConfig {
        bool interactive;
        bool show_matrices;
        bool show_calculations;
        bool step_by_step;
        TutorialLevel detail_level;
        
        TutorialConfig() 
            : interactive(true)
            , show_matrices(true)
            , show_calculations(true)
            , step_by_step(true)
            , detail_level(TutorialLevel::Intermediate)
        {
        }
    };

    /**
     * @brief Constructor
     * @param config Tutorial behavior configuration
     */
    XORTutorial(const TutorialConfig& config = TutorialConfig());

    /**
     * @brief Main tutorial methods
     */
    void runFullTutorial();
    void runInteractive();
    bool validateXORSolution() const;

    /**
     * @brief Individual tutorial sections
     */
    void explainXORProblem();
    void demonstratePerceptronFailure();
    void buildNeuralNetworkSolution();
    void testNetworkOnAllInputs();
    void explainWhyItWorks();

    /**
     * @brief Create pre-trained XOR network with known solution
     */
    NeuralNetwork createPretrainedXORNetwork() const;

    /**
     * @brief Create simple perceptron (single layer) for failure demonstration
     */
    NeuralNetwork createSimplePerceptron();

    /**
     * @brief Training data management
     */
    std::vector<TrainingSample> createXORTrainingData() const;
    void showTrainingData() const;

    /**
     * @brief Validation and testing
     */
    bool testNetwork(const NeuralNetwork& network, double tolerance = 0.1) const;
    void showPredictionResults(const NeuralNetwork& network) const;

    /**
     * @brief JSON export for integration with SupraFit
     */
    QJsonObject exportTutorialResults() const;

private:
    TutorialConfig config_;
    std::vector<TrainingSample> training_data_;
    
    /**
     * @brief Educational helper functions
     */
    void showStep(const QString& title, const QString& explanation);
    void showMathematicalExplanation();
    void demonstrateDecisionBoundary();
    void explainActivationFunctions();
    void showWeightInterpretation(const NeuralNetwork& network);
};