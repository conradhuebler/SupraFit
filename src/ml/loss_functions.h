/*
 * Loss Functions for Neural Network Training in SupraFit
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

#pragma once

#include <Eigen/Dense>
#include <QString>
#include <memory>
#include <cmath>

/**
 * @brief Base class for loss functions used in neural network training
 * 
 * Loss functions measure the difference between predicted and actual values,
 * providing both the loss value and gradient for backpropagation.
 * 
 * Educational Focus: Each loss function includes scientific interpretation
 * and guidance for when to use in chemistry applications.
 * 
 * Claude Generated - 2025
 */
class LossFunction {
public:
    virtual ~LossFunction() = default;
    
    /**
     * @brief Calculate loss value
     * @param predictions Network output predictions
     * @param targets True target values
     * @return Scalar loss value
     */
    virtual double forward(const Eigen::VectorXd& predictions, const Eigen::VectorXd& targets) const = 0;
    
    /**
     * @brief Calculate gradient of loss with respect to predictions
     * @param predictions Network output predictions
     * @param targets True target values
     * @return Gradient vector for backpropagation
     */
    virtual Eigen::VectorXd backward(const Eigen::VectorXd& predictions, const Eigen::VectorXd& targets) const = 0;
    
    /**
     * @brief Human-readable name of loss function
     */
    virtual QString name() const = 0;
    
    /**
     * @brief Scientific explanation and use cases
     */
    virtual QString explanation() const = 0;
};

/**
 * @brief Mean Squared Error (MSE) loss function
 * 
 * Mathematical form: L = (1/n) * Σ(y_pred - y_true)²
 * Best for regression problems where you want to minimize absolute differences.
 * 
 * Chemistry Applications:
 * - Binding constant prediction
 * - Chemical shift regression
 * - Concentration estimation
 */
class MeanSquaredErrorLoss : public LossFunction {
public:
    double forward(const Eigen::VectorXd& predictions, const Eigen::VectorXd& targets) const override {
        if (predictions.size() != targets.size()) {
            throw std::invalid_argument("Predictions and targets must have same size");
        }
        
        Eigen::VectorXd diff = predictions - targets;
        return 0.5 * diff.squaredNorm() / predictions.size();  // 0.5 factor for cleaner gradients
    }
    
    Eigen::VectorXd backward(const Eigen::VectorXd& predictions, const Eigen::VectorXd& targets) const override {
        if (predictions.size() != targets.size()) {
            throw std::invalid_argument("Predictions and targets must have same size");
        }
        
        return (predictions - targets) / predictions.size();
    }
    
    QString name() const override {
        return "Mean Squared Error";
    }
    
    QString explanation() const override {
        return "MSE: L = (1/n) * Σ(y_pred - y_true)². Good for regression, penalizes large errors heavily.";
    }
};

/**
 * @brief Cross Entropy loss function for classification
 * 
 * Mathematical form: L = -Σ(y_true * log(y_pred))
 * Best for classification problems with probabilistic outputs.
 * 
 * Chemistry Applications:
 * - NMR model classification (1:1, 1:2, etc.)
 * - Compound identification
 * - Reaction outcome prediction
 */
class CrossEntropyLoss : public LossFunction {
public:
    double forward(const Eigen::VectorXd& predictions, const Eigen::VectorXd& targets) const override {
        if (predictions.size() != targets.size()) {
            throw std::invalid_argument("Predictions and targets must have same size");
        }
        
        double loss = 0.0;
        const double epsilon = 1e-15;  // Prevent log(0)
        
        for (int i = 0; i < predictions.size(); ++i) {
            double p = std::max(epsilon, std::min(1.0 - epsilon, predictions(i)));
            loss -= targets(i) * std::log(p);
        }
        
        return loss;
    }
    
    Eigen::VectorXd backward(const Eigen::VectorXd& predictions, const Eigen::VectorXd& targets) const override {
        if (predictions.size() != targets.size()) {
            throw std::invalid_argument("Predictions and targets must have same size");
        }
        
        const double epsilon = 1e-15;
        Eigen::VectorXd gradient(predictions.size());
        
        for (int i = 0; i < predictions.size(); ++i) {
            double p = std::max(epsilon, std::min(1.0 - epsilon, predictions(i)));
            gradient(i) = -targets(i) / p;
        }
        
        return gradient;
    }
    
    QString name() const override {
        return "Cross Entropy";
    }
    
    QString explanation() const override {
        return "Cross Entropy: L = -Σ(y_true * log(y_pred)). Ideal for classification with probabilistic outputs.";
    }
};

/**
 * @brief Binary Cross Entropy for binary classification
 * 
 * Specialized version for two-class problems.
 * 
 * Chemistry Applications:
 * - Binding/non-binding classification
 * - Active/inactive compound screening
 * - Pass/fail quality control
 */
class BinaryCrossEntropyLoss : public LossFunction {
public:
    double forward(const Eigen::VectorXd& predictions, const Eigen::VectorXd& targets) const override {
        if (predictions.size() != targets.size()) {
            throw std::invalid_argument("Predictions and targets must have same size");
        }
        
        double loss = 0.0;
        const double epsilon = 1e-15;
        
        for (int i = 0; i < predictions.size(); ++i) {
            double p = std::max(epsilon, std::min(1.0 - epsilon, predictions(i)));
            loss -= targets(i) * std::log(p) + (1 - targets(i)) * std::log(1 - p);
        }
        
        return loss / predictions.size();
    }
    
    Eigen::VectorXd backward(const Eigen::VectorXd& predictions, const Eigen::VectorXd& targets) const override {
        if (predictions.size() != targets.size()) {
            throw std::invalid_argument("Predictions and targets must have same size");
        }
        
        const double epsilon = 1e-15;
        Eigen::VectorXd gradient(predictions.size());
        
        for (int i = 0; i < predictions.size(); ++i) {
            double p = std::max(epsilon, std::min(1.0 - epsilon, predictions(i)));
            gradient(i) = (-targets(i) / p + (1 - targets(i)) / (1 - p)) / predictions.size();
        }
        
        return gradient;
    }
    
    QString name() const override {
        return "Binary Cross Entropy";
    }
    
    QString explanation() const override {
        return "Binary Cross Entropy: L = -[y*log(p) + (1-y)*log(1-p)]. Optimized for binary classification.";
    }
};

/**
 * @brief Factory function for creating loss functions
 * @param name Loss function name ("mse", "crossentropy", "binarycrossentropy")
 * @return Unique pointer to loss function
 */
std::unique_ptr<LossFunction> createLossFunction(const QString& name);

/**
 * @brief Training data structure for supervised learning
 */
struct TrainingData {
    std::vector<Eigen::VectorXd> inputs;
    std::vector<Eigen::VectorXd> targets;
    
    size_t size() const { return inputs.size(); }
    
    /**
     * @brief Create mini-batches for efficient training
     */
    std::vector<std::pair<std::vector<Eigen::VectorXd>, std::vector<Eigen::VectorXd>>> 
    createMiniBatches(size_t batch_size) const;
};