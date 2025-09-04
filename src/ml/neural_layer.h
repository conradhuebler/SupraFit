/*
 * Neural Network Layer Implementation for SupraFit
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

#include <memory>
#include <random>
#include <QString>
#include <QJsonObject>
#include <Eigen/Dense>

#include "activation_functions.h"

/**
 * @brief Forward declaration for tutorial support
 */
class TutorialManager;

/**
 * @brief Neural Network Layer with Educational Focus
 * 
 * Implements a fully connected (dense) neural network layer using Eigen
 * for efficient matrix operations. Designed with educational transparency
 * to support interactive tutorials and step-by-step learning.
 * 
 * Mathematical foundation:
 * output = activation(weights * input + biases)
 * 
 * Where:
 * - weights: [output_size × input_size] matrix
 * - input: [input_size × 1] vector  
 * - biases: [output_size × 1] vector
 * - output: [output_size × 1] vector
 * 
 * Supports multiple weight initialization strategies for different
 * activation functions and network architectures.
 * 
 * Claude Generated - 2025
 */
class NeuralLayer {
public:
    /**
     * @brief Weight initialization methods
     */
    enum class InitializationMethod {
        Xavier,      // Xavier/Glorot: sqrt(6/(fan_in + fan_out))
        He,          // He initialization: sqrt(2/fan_in) - good for ReLU
        Random,      // Simple random [-0.5, 0.5]
        Zero         // All zeros (for testing only)
    };

    /**
     * @brief Construct neural layer
     * @param input_size Number of input neurons
     * @param output_size Number of output neurons
     * @param activation Activation function (takes ownership)
     * @param init_method Weight initialization method
     */
    NeuralLayer(int input_size, int output_size, 
                std::unique_ptr<ActivationFunction> activation,
                InitializationMethod init_method = InitializationMethod::Xavier);

    /**
     * @brief Copy constructor
     */
    NeuralLayer(const NeuralLayer& other);
    
    /**
     * @brief Assignment operator
     */
    NeuralLayer& operator=(const NeuralLayer& other);
    
    /**
     * @brief Destructor
     */
    ~NeuralLayer() = default;

    /**
     * @brief Forward propagation through layer
     * @param input Input vector [input_size × 1]
     * @return Output vector [output_size × 1]
     */
    Eigen::VectorXd forward(const Eigen::VectorXd& input);
    
    /**
     * @brief Backward propagation (backpropagation) through layer
     * @param layer_input Input that was used in forward pass
     * @param output_gradient Gradient from next layer
     * @param weight_gradients Output: gradients w.r.t. weights
     * @param bias_gradients Output: gradients w.r.t. biases
     * @param input_gradient Output: gradients w.r.t. input (for previous layer)
     */
    void backward(const Eigen::VectorXd& layer_input,
                  const Eigen::VectorXd& output_gradient,
                  Eigen::MatrixXd& weight_gradients,
                  Eigen::VectorXd& bias_gradients,
                  Eigen::VectorXd& input_gradient);

    /**
     * @brief Get layer dimensions
     */
    int inputSize() const { return input_size_; }
    int outputSize() const { return output_size_; }

    /**
     * @brief Access weight matrix (read-only)
     */
    const Eigen::MatrixXd& weights() const { return weights_; }
    const Eigen::MatrixXd& getWeights() const { return weights_; }
    
    /**
     * @brief Access bias vector (read-only)  
     */
    const Eigen::VectorXd& biases() const { return biases_; }
    const Eigen::VectorXd& getBiases() const { return biases_; }

    /**
     * @brief Access last forward pass results (for tutorials)
     */
    const Eigen::VectorXd& lastInput() const { return last_input_; }
    const Eigen::VectorXd& lastPreActivation() const { return last_pre_activation_; }
    const Eigen::VectorXd& lastOutput() const { return last_output_; }

    /**
     * @brief Get activation function name
     */
    QString activationName() const { return activation_->name(); }

    /**
     * @brief JSON serialization for model persistence
     */
    QJsonObject toJson() const;
    bool fromJson(const QJsonObject& json);

    /**
     * @brief Educational/debugging information
     */
    QString getLayerInfo() const;
    void printWeights() const;
    void printBiases() const;

    /**
     * @brief Tutorial support
     */
    void enableTutorialMode(TutorialManager* tutorial);
    void disableTutorialMode();

    /**
     * @brief Manual weight/bias setting (for testing and tutorials)
     */
    void setWeights(const Eigen::MatrixXd& weights);
    void setBiases(const Eigen::VectorXd& biases);

    /**
     * @brief Weight initialization methods
     */
    void initializeWeights(InitializationMethod method = InitializationMethod::Xavier,
                          unsigned int seed = 0);

private:
    // Layer dimensions
    int input_size_;
    int output_size_;
    
    // Layer parameters
    Eigen::MatrixXd weights_;  // [output_size × input_size]
    Eigen::VectorXd biases_;   // [output_size × 1]
    
    // Activation function
    std::unique_ptr<ActivationFunction> activation_;
    
    // Tutorial support
    TutorialManager* tutorial_manager_;
    
    // For debugging and tutorials - store last forward pass
    Eigen::VectorXd last_input_;
    Eigen::VectorXd last_pre_activation_;  // Before activation
    Eigen::VectorXd last_output_;         // After activation
    
    /**
     * @brief Apply activation function to vector
     * @param pre_activation Vector before activation
     * @return Vector after activation
     */
    Eigen::VectorXd applyActivation(const Eigen::VectorXd& pre_activation);
    
    /**
     * @brief Weight initialization implementations
     */
    void initializeXavier(unsigned int seed);
    void initializeHe(unsigned int seed);
    void initializeRandom(unsigned int seed);
    void initializeZero();
    
    /**
     * @brief Tutorial helper functions
     */
    void showForwardPassDetails(const Eigen::VectorXd& input,
                               const Eigen::VectorXd& pre_activation,
                               const Eigen::VectorXd& output);
};