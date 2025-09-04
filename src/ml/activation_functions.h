/*
 * Neural Network Activation Functions for SupraFit
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
#include <cmath>
#include <QString>
#include <Eigen/Dense>

/**
 * @brief Base class for neural network activation functions
 * 
 * Provides mathematical foundation for neural network computation with
 * educational transparency. Each activation function includes both forward
 * computation and derivative calculation for potential training implementation.
 * 
 * Design follows educational-first principle with clear scientific meaning
 * and mathematical interpretation accessible for chemistry/physics users.
 * 
 * Claude Generated - 2025
 */
class ActivationFunction {
public:
    virtual ~ActivationFunction() = default;
    
    /**
     * @brief Forward activation computation
     * @param x Input value
     * @return Activated output value
     */
    virtual double forward(double x) const = 0;
    
    /**
     * @brief Derivative computation (for backpropagation)
     * @param x Input value
     * @return Derivative value at x
     */
    virtual double backward(double x) const = 0;
    
    /**
     * @brief Human-readable name of activation function
     * @return Function name for debugging and tutorials
     */
    virtual QString name() const = 0;
    
    /**
     * @brief Educational explanation of function purpose
     * @return Scientific/mathematical explanation for tutorials
     */
    virtual QString explanation() const = 0;
    
    /**
     * @brief Typical use case in neural networks
     * @return When and why to use this activation function
     */
    virtual QString useCase() const = 0;
    
    /**
     * @brief Vector-wise forward pass (for testing and bulk operations)
     * @param input Input vector
     * @return Output vector with activation applied element-wise
     */
    virtual Eigen::VectorXd forwardVector(const Eigen::VectorXd& input) const {
        Eigen::VectorXd result(input.size());
        for (int i = 0; i < input.size(); ++i) {
            result(i) = forward(input(i));
        }
        return result;
    }
};

/**
 * @brief Rectified Linear Unit (ReLU) activation function
 * 
 * Mathematical form: f(x) = max(0, x)
 * Most commonly used activation function in modern neural networks.
 * Provides non-linearity while avoiding vanishing gradient problems.
 * 
 * Scientific interpretation: Acts like a "switch" that passes positive
 * signals and blocks negative ones, similar to enzyme activity thresholds.
 */
class ReLUActivation : public ActivationFunction {
public:
    double forward(double x) const override {
        return std::max(0.0, x);
    }
    
    double backward(double x) const override {
        return x > 0.0 ? 1.0 : 0.0;
    }
    
    QString name() const override {
        return "ReLU";
    }
    
    QString explanation() const override {
        return "Rectified Linear Unit: f(x) = max(0, x). Passes positive values unchanged, blocks negative values.";
    }
    
    QString useCase() const override {
        return "Hidden layers: Avoids vanishing gradients, fast computation, sparse activation (biological realism).";
    }
};

/**
 * @brief Sigmoid activation function
 * 
 * Mathematical form: f(x) = 1 / (1 + e^(-x))
 * Classic activation function providing smooth transition between 0 and 1.
 * Useful for probability interpretation and binary classification.
 * 
 * Scientific interpretation: S-shaped curve resembling binding isotherms
 * in chemistry (Hill equation) or dose-response curves in pharmacology.
 */
class SigmoidActivation : public ActivationFunction {
public:
    double forward(double x) const override {
        // Clip input to prevent overflow/underflow
        double clipped_x = std::max(-500.0, std::min(500.0, x));
        double result = 1.0 / (1.0 + std::exp(-clipped_x));
        // Ensure we never reach exactly 1.0 or 0.0 for numerical stability
        return std::max(1e-15, std::min(1.0 - 1e-15, result));
    }
    
    double backward(double x) const override {
        double s = forward(x);
        return s * (1.0 - s);
    }
    
    QString name() const override {
        return "Sigmoid";
    }
    
    QString explanation() const override {
        return "Sigmoid: f(x) = 1/(1+e^(-x)). Smooth S-curve mapping any input to range [0,1].";
    }
    
    QString useCase() const override {
        return "Output layer for binary classification, probability interpretation, gate mechanisms.";
    }
};

/**
 * @brief Hyperbolic Tangent (tanh) activation function
 * 
 * Mathematical form: f(x) = (e^x - e^(-x)) / (e^x + e^(-x))
 * Symmetric around origin, outputs in range [-1, 1].
 * Often performs better than sigmoid due to zero-centered output.
 */
class TanhActivation : public ActivationFunction {
public:
    double forward(double x) const override {
        // Clip input to prevent overflow/underflow
        double clipped_x = std::max(-500.0, std::min(500.0, x));
        return std::tanh(clipped_x);
    }
    
    double backward(double x) const override {
        double t = forward(x);
        return 1.0 - t * t;
    }
    
    QString name() const override {
        return "Tanh";
    }
    
    QString explanation() const override {
        return "Hyperbolic Tangent: f(x) = tanh(x). S-curve centered at origin, output range [-1,1].";
    }
    
    QString useCase() const override {
        return "Hidden layers when zero-centered output is desired, RNNs, traditional neural networks.";
    }
};

/**
 * @brief Softmax activation function
 * 
 * Mathematical form: f(x_i) = e^(x_i) / Σ(e^(x_j))
 * Multi-class generalization of sigmoid, outputs probability distribution.
 * Applied to entire layer output, not individual neurons.
 * 
 * Scientific interpretation: Boltzmann distribution from statistical mechanics,
 * similar to relative populations of energy states in thermodynamics.
 */
class SoftmaxActivation : public ActivationFunction {
public:
    /**
     * @brief Apply softmax to entire vector (layer output)
     * @param x Input vector
     * @return Probability distribution vector
     */
    Eigen::VectorXd forwardVector(const Eigen::VectorXd& x) const override {
        // Handle empty vector case
        if (x.size() == 0) {
            return Eigen::VectorXd(0);
        }
        
        // Subtract max for numerical stability (prevents overflow)
        double max_val = x.maxCoeff();
        Eigen::VectorXd exp_x = (x.array() - max_val).exp();
        double sum_exp = exp_x.sum();
        
        // Handle edge case where sum is zero or infinite
        if (sum_exp <= 0.0 || !std::isfinite(sum_exp)) {
            // Return uniform distribution
            return Eigen::VectorXd::Constant(x.size(), 1.0 / x.size());
        }
        
        return exp_x / sum_exp;
    }
    
    // Individual element functions (not typically used for softmax)
    double forward(double x) const override {
        return x; // Not meaningful for individual elements
    }
    
    double backward(double /*x*/) const override {
        return 1.0; // Complex Jacobian matrix needed for full derivative
    }
    
    QString name() const override {
        return "Softmax";
    }
    
    QString explanation() const override {
        return "Softmax: f(x_i) = e^(x_i)/Σe^(x_j). Converts logits to probability distribution.";
    }
    
    QString useCase() const override {
        return "Output layer for multi-class classification, model selection, probability distributions.";
    }
};

/**
 * @brief Linear activation function (identity)
 * 
 * Mathematical form: f(x) = x
 * No transformation applied, useful for regression output layers.
 */
class LinearActivation : public ActivationFunction {
public:
    double forward(double x) const override {
        return x;
    }
    
    double backward(double /*x*/) const override {
        return 1.0;
    }
    
    QString name() const override {
        return "Linear";
    }
    
    QString explanation() const override {
        return "Linear: f(x) = x. Identity function, no transformation applied.";
    }
    
    QString useCase() const override {
        return "Output layer for regression tasks, preserving full range of values.";
    }
};

/**
 * @brief Factory function for creating activation functions
 * @param name Activation function name ("relu", "sigmoid", "tanh", "softmax", "linear")
 * @return Unique pointer to activation function
 */
std::unique_ptr<ActivationFunction> createActivationFunction(const QString& name);