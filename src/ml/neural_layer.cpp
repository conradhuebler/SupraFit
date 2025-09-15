/*
 * Neural Network Layer Implementation for SupraFit
 * Copyright (C) 2025 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "neural_layer.h"
#include "tutorial/tutorial_manager.h"

#include <QtCore/QJsonArray>
#include <QtCore/QDebug>
#include <fmt/format.h>

NeuralLayer::NeuralLayer(int input_size, int output_size, 
                        std::unique_ptr<ActivationFunction> activation,
                        InitializationMethod init_method)
    : input_size_(input_size)
    , output_size_(output_size)
    , activation_(std::move(activation))
    , tutorial_manager_(nullptr)
{
    // Initialize matrices
    weights_ = Eigen::MatrixXd::Zero(output_size, input_size);
    biases_ = Eigen::VectorXd::Zero(output_size);
    
    // Initialize storage for tutorial/debugging
    last_input_ = Eigen::VectorXd::Zero(input_size);
    last_pre_activation_ = Eigen::VectorXd::Zero(output_size);
    last_output_ = Eigen::VectorXd::Zero(output_size);
    
    // Initialize weights according to specified method
    initializeWeights(init_method);
}

NeuralLayer::NeuralLayer(const NeuralLayer& other)
    : input_size_(other.input_size_)
    , output_size_(other.output_size_)
    , weights_(other.weights_)
    , biases_(other.biases_)
    , tutorial_manager_(nullptr)  // Don't copy tutorial manager
    , last_input_(other.last_input_)
    , last_pre_activation_(other.last_pre_activation_)
    , last_output_(other.last_output_)
{
    // Deep copy activation function
    if (other.activation_) {
        // Create new activation function of same type
        activation_ = createActivationFunction(other.activation_->name());
    }
}

NeuralLayer& NeuralLayer::operator=(const NeuralLayer& other)
{
    if (this != &other) {
        input_size_ = other.input_size_;
        output_size_ = other.output_size_;
        weights_ = other.weights_;
        biases_ = other.biases_;
        tutorial_manager_ = nullptr;  // Don't copy tutorial manager
        last_input_ = other.last_input_;
        last_pre_activation_ = other.last_pre_activation_;
        last_output_ = other.last_output_;
        
        // Deep copy activation function
        if (other.activation_) {
            activation_ = createActivationFunction(other.activation_->name());
        }
    }
    return *this;
}

Eigen::VectorXd NeuralLayer::forward(const Eigen::VectorXd& input)
{
    // Validate input dimensions
    if (input.size() != input_size_) {
        qDebug() << "Neural Layer Error: Input size mismatch. Expected:" << input_size_ 
                 << "Got:" << input.size();
        return Eigen::VectorXd::Zero(output_size_);
    }
    
    // Store input for tutorials/debugging
    last_input_ = input;
    
    // Matrix multiplication: output = weights * input + biases
    last_pre_activation_ = weights_ * input + biases_;
    
    // Apply activation function
    last_output_ = applyActivation(last_pre_activation_);
    
    // Show tutorial information if enabled
    if (tutorial_manager_) {
        showForwardPassDetails(input, last_pre_activation_, last_output_);
    }
    
    return last_output_;
}

void NeuralLayer::backward(const Eigen::VectorXd& layer_input,
                          const Eigen::VectorXd& output_gradient,
                          Eigen::MatrixXd& weight_gradients,
                          Eigen::VectorXd& bias_gradients,
                          Eigen::VectorXd& input_gradient)
{
    // Compute gradients w.r.t. pre-activation values
    // For most activations: grad_pre = output_gradient * activation_derivative(pre_activation)
    Eigen::VectorXd pre_activation_gradient = output_gradient;
    
    if (activation_) {
        // Apply activation function derivative
        if (activation_->name() == "Softmax") {
            // Softmax derivative is more complex - for now use simplified approach
            pre_activation_gradient = output_gradient;
        } else {
            // Element-wise derivative for most activation functions
            for (int i = 0; i < pre_activation_gradient.size(); ++i) {
                double derivative = activation_->backward(last_pre_activation_(i));
                pre_activation_gradient(i) *= derivative;
            }
        }
    }
    
    // Compute weight gradients: dW = pre_activation_gradient * input^T
    weight_gradients = pre_activation_gradient * layer_input.transpose();
    
    // Compute bias gradients: db = pre_activation_gradient
    bias_gradients = pre_activation_gradient;
    
    // Compute input gradients: dx = W^T * pre_activation_gradient
    input_gradient = weights_.transpose() * pre_activation_gradient;
}

Eigen::VectorXd NeuralLayer::applyActivation(const Eigen::VectorXd& pre_activation)
{
    if (!activation_) {
        return pre_activation; // Linear activation by default
    }
    
    // Special handling for Softmax (operates on entire vector)
    if (activation_->name() == "Softmax") {
        auto* softmax = dynamic_cast<SoftmaxActivation*>(activation_.get());
        if (softmax) {
            return softmax->forwardVector(pre_activation);
        }
    }
    
    // Element-wise activation for other functions
    Eigen::VectorXd activated(pre_activation.size());
    for (int i = 0; i < pre_activation.size(); ++i) {
        activated(i) = activation_->forward(pre_activation(i));
    }
    
    return activated;
}

void NeuralLayer::initializeWeights(InitializationMethod method, unsigned int seed)
{
    switch (method) {
        case InitializationMethod::Xavier:
            initializeXavier(seed);
            break;
        case InitializationMethod::He:
            initializeHe(seed);
            break;
        case InitializationMethod::Random:
            initializeRandom(seed);
            break;
        case InitializationMethod::Zero:
            initializeZero();
            break;
    }
}

void NeuralLayer::initializeXavier(unsigned int seed)
{
    std::mt19937 gen(seed == 0 ? std::random_device{}() : seed);
    
    // Xavier/Glorot initialization: sqrt(6 / (fan_in + fan_out))
    double std_dev = std::sqrt(6.0 / (input_size_ + output_size_));
    std::uniform_real_distribution<double> dist(-std_dev, std_dev);
    
    for (int i = 0; i < weights_.rows(); ++i) {
        for (int j = 0; j < weights_.cols(); ++j) {
            weights_(i, j) = dist(gen);
        }
    }
    
    // Initialize biases to zero (common practice)
    biases_.setZero();
}

void NeuralLayer::initializeHe(unsigned int seed)
{
    std::mt19937 gen(seed == 0 ? std::random_device{}() : seed);
    
    // He initialization: sqrt(2 / fan_in) - good for ReLU
    double std_dev = std::sqrt(2.0 / input_size_);
    std::normal_distribution<double> dist(0.0, std_dev);
    
    for (int i = 0; i < weights_.rows(); ++i) {
        for (int j = 0; j < weights_.cols(); ++j) {
            weights_(i, j) = dist(gen);
        }
    }
    
    biases_.setZero();
}

void NeuralLayer::initializeRandom(unsigned int seed)
{
    std::mt19937 gen(seed == 0 ? std::random_device{}() : seed);
    std::uniform_real_distribution<double> dist(-0.5, 0.5);
    
    for (int i = 0; i < weights_.rows(); ++i) {
        for (int j = 0; j < weights_.cols(); ++j) {
            weights_(i, j) = dist(gen);
        }
    }
    
    // Small random biases
    for (int i = 0; i < biases_.size(); ++i) {
        biases_(i) = dist(gen) * 0.1;  // Smaller scale for biases
    }
}

void NeuralLayer::initializeZero()
{
    weights_.setZero();
    biases_.setZero();
}

void NeuralLayer::setWeights(const Eigen::MatrixXd& weights)
{
    if (weights.rows() == output_size_ && weights.cols() == input_size_) {
        weights_ = weights;
    } else {
        qDebug() << "Neural Layer Error: Weight matrix size mismatch";
    }
}

void NeuralLayer::setBiases(const Eigen::VectorXd& biases)
{
    if (biases.size() == output_size_) {
        biases_ = biases;
    } else {
        qDebug() << "Neural Layer Error: Bias vector size mismatch";
    }
}

QString NeuralLayer::getLayerInfo() const
{
    return QString("Layer [%1 → %2] with %3 activation")
           .arg(input_size_)
           .arg(output_size_)
           .arg(activation_ ? activation_->name() : "None");
}

void NeuralLayer::printWeights() const
{
    qDebug() << "Layer Weights" << weights_.rows() << "×" << weights_.cols() << ":";
    for (int i = 0; i < weights_.rows(); ++i) {
        QString row;
        for (int j = 0; j < weights_.cols(); ++j) {
            row += QString("%1 ").arg(weights_(i, j), 8, 'f', 4);
        }
        qDebug() << "  " << row;
    }
}

void NeuralLayer::printBiases() const
{
    qDebug() << "Layer Biases:";
    QString biasStr;
    for (int i = 0; i < biases_.size(); ++i) {
        biasStr += QString("%1 ").arg(biases_(i), 8, 'f', 4);
    }
    qDebug() << "  " << biasStr;
}

QJsonObject NeuralLayer::toJson() const
{
    QJsonObject json;
    
    // Layer configuration
    json["input_size"] = input_size_;
    json["output_size"] = output_size_;
    json["activation"] = activation_ ? activation_->name() : "linear";
    
    // Weights matrix
    QJsonArray weightsArray;
    for (int i = 0; i < weights_.rows(); ++i) {
        QJsonArray row;
        for (int j = 0; j < weights_.cols(); ++j) {
            row.append(weights_(i, j));
        }
        weightsArray.append(row);
    }
    json["weights"] = weightsArray;
    
    // Biases vector
    QJsonArray biasesArray;
    for (int i = 0; i < biases_.size(); ++i) {
        biasesArray.append(biases_(i));
    }
    json["biases"] = biasesArray;
    
    return json;
}

bool NeuralLayer::fromJson(const QJsonObject& json)
{
    if (!json.contains("input_size") || !json.contains("output_size") ||
        !json.contains("weights") || !json.contains("biases")) {
        return false;
    }
    
    // Load configuration
    input_size_ = json["input_size"].toInt();
    output_size_ = json["output_size"].toInt();
    
    // Create activation function
    QString activationName = json["activation"].toString("linear");
    activation_ = createActivationFunction(activationName);
    
    // Load weights
    QJsonArray weightsArray = json["weights"].toArray();
    weights_ = Eigen::MatrixXd(output_size_, input_size_);
    
    for (int i = 0; i < weightsArray.size() && i < output_size_; ++i) {
        QJsonArray row = weightsArray[i].toArray();
        for (int j = 0; j < row.size() && j < input_size_; ++j) {
            weights_(i, j) = row[j].toDouble();
        }
    }
    
    // Load biases
    QJsonArray biasesArray = json["biases"].toArray();
    biases_ = Eigen::VectorXd(output_size_);
    
    for (int i = 0; i < biasesArray.size() && i < output_size_; ++i) {
        biases_(i) = biasesArray[i].toDouble();
    }
    
    return true;
}

void NeuralLayer::enableTutorialMode(TutorialManager* tutorial)
{
    tutorial_manager_ = tutorial;
}

void NeuralLayer::disableTutorialMode()
{
    tutorial_manager_ = nullptr;
}

void NeuralLayer::showForwardPassDetails(const Eigen::VectorXd& input,
                                        const Eigen::VectorXd& pre_activation,
                                        const Eigen::VectorXd& output)
{
    if (!tutorial_manager_) return;
    
    // This will be implemented when TutorialManager is ready
    // For now, use simple debug output
    qDebug() << "🔄 Layer Forward Pass:";
    qDebug() << "   Input size:" << input.size();
    qDebug() << "   Pre-activation size:" << pre_activation.size();
    qDebug() << "   Output size:" << output.size();
    qDebug() << "   Activation:" << (activation_ ? activation_->name() : "None");
}