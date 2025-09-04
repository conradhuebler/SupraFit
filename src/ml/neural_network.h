/*
 * Neural Network Core Implementation for SupraFit
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
#include <memory>
#include <QString>
#include <QJsonObject>
#include <Eigen/Dense>

#include "neural_layer.h"
#include "tutorial/tutorial_manager.h"
#include "loss_functions.h"
#include "optimizer.h"

/**
 * @brief Feed-Forward Neural Network with Educational Focus
 * 
 * Implements a multi-layer perceptron (MLP) neural network designed
 * for scientific applications in SupraFit. Prioritizes educational
 * transparency and step-by-step learning over performance optimization.
 * 
 * Key Features:
 * - Configurable layer architecture
 * - Multiple activation functions per layer
 * - JSON-based model persistence
 * - Interactive tutorial mode
 * - Educational explanations and visualizations
 * - Integration with SupraFit's statistical framework
 * 
 * Typical usage for NMR model selection:
 * 1. Create network with appropriate architecture (e.g., [10, 8, 4] neurons)
 * 2. Load pre-trained weights or train from scratch
 * 3. Feed MLFeatureExtractor output for model prediction
 * 4. Interpret results as probability distribution over models
 * 
 * Claude Generated - 2025
 */
class NeuralNetwork {
public:
    /**
     * @brief Network performance metrics
     */
    struct PerformanceMetrics {
        double inference_time_ms = 0.0;
        size_t total_parameters = 0;
        double memory_usage_mb = 0.0;
        QString last_prediction_info;
    };

    /**
     * @brief Prediction result with confidence information
     */
    struct PredictionResult {
        Eigen::VectorXd probabilities;    // Class probabilities
        int predicted_class = -1;         // Best prediction
        double confidence = 0.0;          // Max probability
        QString interpretation;           // Human-readable explanation
    };

    /**
     * @brief Default constructor (empty network)
     */
    NeuralNetwork();
    
    /**
     * @brief Construct network with specified architecture
     * @param layer_sizes Vector of layer sizes [input, hidden1, hidden2, ..., output]
     * @param activations Activation function names for each layer (optional)
     */
    NeuralNetwork(const std::vector<int>& layer_sizes, 
                  const std::vector<QString>& activations = {});

    /**
     * @brief Copy constructor
     */
    NeuralNetwork(const NeuralNetwork& other);
    
    /**
     * @brief Assignment operator
     */
    NeuralNetwork& operator=(const NeuralNetwork& other);
    
    /**
     * @brief Destructor
     */
    ~NeuralNetwork() = default;

    /**
     * @brief Network Architecture Management
     */
    void addLayer(int input_size, int output_size, 
                  const QString& activation = "relu",
                  NeuralLayer::InitializationMethod init = NeuralLayer::InitializationMethod::Xavier);
    void clearLayers();
    size_t layerCount() const { return layers_.size(); }
    const NeuralLayer& getLayer(size_t index) const;

    /**
     * @brief Forward Propagation (Main Inference)
     * @param input Input feature vector
     * @return Network output (logits or probabilities)
     */
    Eigen::VectorXd predict(const Eigen::VectorXd& input);
    
    /**
     * @brief Forward propagation with detailed results
     * @param input Input feature vector
     * @return Structured prediction result with confidence and interpretation
     */
    PredictionResult predictWithDetails(const Eigen::VectorXd& input);

    /**
     * @brief Neural Network Training (Claude Generated - 2025)
     */
    struct TrainingHistory {
        std::vector<double> train_loss;
        std::vector<double> validation_loss;
        std::vector<double> train_accuracy;
        std::vector<double> validation_accuracy;
        int epochs_completed;
        
        TrainingHistory() : epochs_completed(0) {}
    };
    
    /**
     * @brief Train the neural network on data
     * @param training_data Input/target pairs for training
     * @param config Training configuration (epochs, learning rate, etc.)
     * @return Training history with loss and accuracy curves
     */
    TrainingHistory train(const TrainingData& training_data, const TrainingConfig& config);
    
    /**
     * @brief Backpropagation algorithm implementation
     * @param input Network input
     * @param target Expected output
     * @param loss_function Loss function to minimize
     * @return Total loss value
     */
    double backpropagate(const Eigen::VectorXd& input, const Eigen::VectorXd& target, 
                        const LossFunction& loss_function);
    
    /**
     * @brief Calculate accuracy on a dataset
     * @param data Test data
     * @param threshold Classification threshold (for binary classification)
     * @return Accuracy percentage (0.0 to 1.0)
     */
    double calculateAccuracy(const TrainingData& data, double threshold = 0.5) const;

    /**
     * @brief Model Persistence (SupraFit JSON Format)
     */
    QJsonObject saveModel() const;
    bool loadModel(const QJsonObject& json);
    bool saveToFile(const QString& filename) const;
    bool loadFromFile(const QString& filename);

    /**
     * @brief Network Information and Statistics
     */
    QString getNetworkInfo() const;
    std::vector<int> getArchitecture() const;
    size_t getTotalParameters() const;
    double getMemoryUsageMB() const;
    const PerformanceMetrics& getPerformanceMetrics() const { return metrics_; }

    /**
     * @brief Educational and Tutorial Features
     */
    void enableTutorialMode(TutorialLevel level = TutorialLevel::Basic, 
                           bool interactive = true);
    void disableTutorialMode();
    bool isTutorialEnabled() const { return tutorial_manager_ != nullptr; }
    
    void showNetworkArchitecture() const;

    /**
     * @brief Debugging and Validation
     */
    bool validateArchitecture() const;
    void printNetworkSummary() const;
    void printWeightsAndBiases() const;
    void showLayerDetails(size_t layer_index) const;
    void showLastPredictionDetails() const;
    
    /**
     * @brief Set custom layer weights (for testing/tutorials)
     */
    void setLayerWeights(size_t layer_index, const Eigen::MatrixXd& weights);
    void setLayerBiases(size_t layer_index, const Eigen::VectorXd& biases);

    /**
     * @brief Performance benchmarking
     */
    void benchmarkInference(const std::vector<Eigen::VectorXd>& test_inputs, 
                           int iterations = 100);

    /**
     * @brief Error handling
     */
    enum class ErrorCode {
        Success,
        InvalidInput,
        EmptyNetwork,
        DimensionMismatch,
        FileError,
        JsonError,
        InvalidState
    };
    
    ErrorCode getLastError() const { return last_error_; }
    QString getLastErrorMessage() const { return last_error_message_; }

private:
    // Network architecture
    std::vector<std::unique_ptr<NeuralLayer>> layers_;
    
    // Tutorial support
    std::unique_ptr<TutorialManager> tutorial_manager_;
    
    // Performance tracking
    PerformanceMetrics metrics_;
    
    // Error handling
    mutable ErrorCode last_error_;
    mutable QString last_error_message_;
    
    // For debugging and tutorials
    std::vector<Eigen::VectorXd> layer_outputs_;
    PredictionResult last_prediction_;

    /**
     * @brief Internal helper functions
     */
    void updatePerformanceMetrics(double inference_time_ms);
    void validateInputDimensions(const Eigen::VectorXd& input) const;
    void setError(ErrorCode code, const QString& message) const;
    void clearError() const;
    
    /**
     * @brief Tutorial helper functions
     */
    void showInputProcessing(const Eigen::VectorXd& input) const;
    void showLayerTransition(size_t layer_index, 
                           const Eigen::VectorXd& input,
                           const Eigen::VectorXd& output) const;
    void showFinalOutput(const Eigen::VectorXd& output) const;
    QString interpretPrediction(const Eigen::VectorXd& output) const;
};