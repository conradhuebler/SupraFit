/*
 * Neural Network Optimizers for SupraFit
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

#include <Eigen/Dense>
#include <QString>
#include <memory>
#include <vector>

/**
 * @brief Base class for neural network optimizers
 * 
 * Optimizers update network weights based on gradients computed during backpropagation.
 * Different optimizers have different convergence properties and are suited for different
 * types of problems in chemistry.
 * 
 * Claude Generated - 2025
 */
class Optimizer {
public:
    virtual ~Optimizer() = default;
    
    /**
     * @brief Update weights based on gradients
     * @param weights Current layer weights
     * @param weight_gradients Gradients w.r.t. weights
     * @param biases Current layer biases
     * @param bias_gradients Gradients w.r.t. biases
     */
    virtual void update(Eigen::MatrixXd& weights, const Eigen::MatrixXd& weight_gradients,
                       Eigen::VectorXd& biases, const Eigen::VectorXd& bias_gradients) = 0;
    
    /**
     * @brief Reset optimizer state (for momentum-based optimizers)
     */
    virtual void reset() = 0;
    
    /**
     * @brief Get optimizer name and parameters
     */
    virtual QString name() const = 0;
    virtual QString description() const = 0;
};

/**
 * @brief Stochastic Gradient Descent (SGD) optimizer
 * 
 * The simplest optimizer: weights = weights - learning_rate * gradient
 * 
 * Good for:
 * - Educational purposes (easy to understand)
 * - Simple problems with well-behaved loss surfaces
 * - When you need reproducible, predictable behavior
 */
class SGDOptimizer : public Optimizer {
public:
    explicit SGDOptimizer(double learning_rate = 0.01) 
        : learning_rate_(learning_rate) {}
    
    void update(Eigen::MatrixXd& weights, const Eigen::MatrixXd& weight_gradients,
               Eigen::VectorXd& biases, const Eigen::VectorXd& bias_gradients) override {
        weights -= learning_rate_ * weight_gradients;
        biases -= learning_rate_ * bias_gradients;
    }
    
    void reset() override {
        // SGD has no internal state to reset
    }
    
    QString name() const override {
        return QString("SGD (lr=%1)").arg(learning_rate_);
    }
    
    QString description() const override {
        return "Stochastic Gradient Descent: Simple, predictable weight updates";
    }
    
    void setLearningRate(double lr) { learning_rate_ = lr; }
    double getLearningRate() const { return learning_rate_; }

private:
    double learning_rate_;
};

/**
 * @brief SGD with Momentum optimizer
 * 
 * Adds momentum to help escape local minima and accelerate convergence.
 * velocity = momentum * velocity - learning_rate * gradient
 * weights = weights + velocity
 * 
 * Good for:
 * - Faster convergence than plain SGD
 * - Smoother training curves
 * - Chemistry problems with noisy gradients
 */
class MomentumOptimizer : public Optimizer {
public:
    explicit MomentumOptimizer(double learning_rate = 0.01, double momentum = 0.9)
        : learning_rate_(learning_rate), momentum_(momentum), initialized_(false) {}
    
    void update(Eigen::MatrixXd& weights, const Eigen::MatrixXd& weight_gradients,
               Eigen::VectorXd& biases, const Eigen::VectorXd& bias_gradients) override {
        
        // Initialize velocity arrays on first call
        if (!initialized_) {
            weight_velocity_ = Eigen::MatrixXd::Zero(weights.rows(), weights.cols());
            bias_velocity_ = Eigen::VectorXd::Zero(biases.size());
            initialized_ = true;
        }
        
        // Update velocities
        weight_velocity_ = momentum_ * weight_velocity_ - learning_rate_ * weight_gradients;
        bias_velocity_ = momentum_ * bias_velocity_ - learning_rate_ * bias_gradients;
        
        // Update parameters
        weights += weight_velocity_;
        biases += bias_velocity_;
    }
    
    void reset() override {
        initialized_ = false;
        weight_velocity_.setZero();
        bias_velocity_.setZero();
    }
    
    QString name() const override {
        return QString("Momentum (lr=%1, m=%2)").arg(learning_rate_).arg(momentum_);
    }
    
    QString description() const override {
        return "SGD with Momentum: Accelerated convergence, smoother training";
    }

private:
    double learning_rate_;
    double momentum_;
    bool initialized_;
    Eigen::MatrixXd weight_velocity_;
    Eigen::VectorXd bias_velocity_;
};

/**
 * @brief Adam optimizer
 * 
 * Adaptive learning rate optimizer that combines momentum with per-parameter learning rates.
 * Generally the best all-around optimizer for most neural network problems.
 * 
 * Good for:
 * - Most chemistry ML problems
 * - When you want "set and forget" optimization
 * - Noisy or sparse gradients
 * - Fast initial convergence
 */
class AdamOptimizer : public Optimizer {
public:
    explicit AdamOptimizer(double learning_rate = 0.001, double beta1 = 0.9, 
                          double beta2 = 0.999, double epsilon = 1e-8)
        : learning_rate_(learning_rate), beta1_(beta1), beta2_(beta2), 
          epsilon_(epsilon), t_(0), initialized_(false) {}
    
    void update(Eigen::MatrixXd& weights, const Eigen::MatrixXd& weight_gradients,
               Eigen::VectorXd& biases, const Eigen::VectorXd& bias_gradients) override {
        
        // Initialize moment arrays on first call
        if (!initialized_) {
            m_weights_ = Eigen::MatrixXd::Zero(weights.rows(), weights.cols());
            v_weights_ = Eigen::MatrixXd::Zero(weights.rows(), weights.cols());
            m_biases_ = Eigen::VectorXd::Zero(biases.size());
            v_biases_ = Eigen::VectorXd::Zero(biases.size());
            initialized_ = true;
        }
        
        t_++;  // Time step
        
        // Update biased first moment estimates
        m_weights_ = beta1_ * m_weights_ + (1 - beta1_) * weight_gradients;
        m_biases_ = beta1_ * m_biases_ + (1 - beta1_) * bias_gradients;
        
        // Update biased second moment estimates
        v_weights_ = beta2_ * v_weights_ + (1 - beta2_) * weight_gradients.array().square().matrix();
        v_biases_ = beta2_ * v_biases_ + (1 - beta2_) * bias_gradients.array().square().matrix();
        
        // Compute bias-corrected estimates
        double alpha_t = learning_rate_ * std::sqrt(1 - std::pow(beta2_, t_)) / (1 - std::pow(beta1_, t_));
        
        // Update parameters
        weights.array() -= alpha_t * m_weights_.array() / (v_weights_.array().sqrt() + epsilon_);
        biases.array() -= alpha_t * m_biases_.array() / (v_biases_.array().sqrt() + epsilon_);
    }
    
    void reset() override {
        t_ = 0;
        initialized_ = false;
        m_weights_.setZero();
        v_weights_.setZero();
        m_biases_.setZero();
        v_biases_.setZero();
    }
    
    QString name() const override {
        return QString("Adam (lr=%1)").arg(learning_rate_);
    }
    
    QString description() const override {
        return "Adam: Adaptive learning rates, excellent for most problems";
    }

private:
    double learning_rate_;
    double beta1_, beta2_, epsilon_;
    int t_;  // Time step
    bool initialized_;
    Eigen::MatrixXd m_weights_, v_weights_;  // Weight moments
    Eigen::VectorXd m_biases_, v_biases_;    // Bias moments
};

/**
 * @brief Factory function for creating optimizers
 * @param name Optimizer name ("sgd", "momentum", "adam")
 * @param learning_rate Learning rate parameter
 * @return Unique pointer to optimizer
 */
std::unique_ptr<Optimizer> createOptimizer(const QString& name, double learning_rate = 0.01);

/**
 * @brief Training configuration structure
 */
struct TrainingConfig {
    int epochs;
    int batch_size;
    double learning_rate;
    QString optimizer_name;
    QString loss_function;
    bool shuffle_data;
    int print_every;  // Print loss every N epochs
    double validation_split;  // Fraction of data for validation
    
    TrainingConfig()
        : epochs(100)
        , batch_size(32)
        , learning_rate(0.01)
        , optimizer_name("adam")
        , loss_function("mse")
        , shuffle_data(true)
        , print_every(10)
        , validation_split(0.2)
    {
    }
};