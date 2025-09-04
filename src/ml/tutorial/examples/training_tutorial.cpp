/*
 * Neural Network Training Tutorial Implementation
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

#include "training_tutorial.h"
#include <fmt/format.h>
#include <fmt/color.h>
#include <QThread>
#include <cmath>
#include <random>

TrainingTutorial::TrainingTutorial(const TrainingTutorialConfig& config)
    : config_(config)
{
}

void TrainingTutorial::runFullTutorial()
{
    fmt::print("\n");
    fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::cyan), 
               "🧠 SupraFit Neural Network Training Tutorial\n");
    fmt::print("===============================================\n\n");
    
    if (config_.show_data_preparation) {
        prepareTrainingData();
    }
    
    if (config_.show_training_details) {
        trainXORFromScratch();
    }
    
    if (config_.show_hyperparameter_effects) {
        demonstrateHyperparameters();
    }
    
    if (config_.show_chemical_interpretation) {
        explainChemistryApplications();
    }
    
    compareTrainingResults();
    
    fmt::print("\n");
    fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::green),
               "✅ Training Tutorial Complete!\n");
    fmt::print("You now understand neural network training fundamentals.\n\n");
}

void TrainingTutorial::runInteractive()
{
    fmt::print("\n");
    fmt::print(fmt::emphasis::bold, "🎓 Interactive Training Tutorial\n");
    fmt::print("==================================\n");
    
    if (config_.interactive) {
        fmt::print("\nPress Enter after each section to continue...\n");
        std::cin.get();
    }
    
    runFullTutorial();
}

void TrainingTutorial::explainTrainingProcess()
{
    showTrainingStep("Understanding Neural Network Training", 
        "Neural networks learn by adjusting weights to minimize prediction errors.\n"
        "This process involves:\n"
        "1. Forward pass: Data flows through the network to make predictions\n"
        "2. Loss calculation: Compare predictions to actual values\n"
        "3. Backward pass: Calculate gradients (how to change weights)\n"
        "4. Weight update: Adjust weights using an optimizer\n"
        "5. Repeat until the network learns the pattern"
    );
}

void TrainingTutorial::prepareTrainingData()
{
    showTrainingStep("Data Preparation", 
        "Good training data is crucial for neural network success.\n"
        "Key principles:\n"
        "• Sufficient examples (hundreds to thousands)\n"
        "• Balanced classes (equal representation)\n"
        "• Feature normalization (scale inputs to [0,1] or [-1,1])\n"
        "• Train/validation split (80/20 is common)"
    );
    
    auto xor_data = createXORData();
    showDataDistribution(xor_data);
    
    if (config_.detail_level >= TutorialLevel::Advanced) {
        auto binding_data = createBindingData();
        fmt::print("\n📊 Chemistry Example: Binding Affinity Data\n");
        showDataDistribution(binding_data);
    }
}

void TrainingTutorial::trainXORFromScratch()
{
    showTrainingStep("Training XOR from Scratch", 
        "XOR is a classic example that demonstrates why neural networks need hidden layers.\n"
        "XOR truth table: (0,0)→0, (0,1)→1, (1,0)→1, (1,1)→0\n"
        "This is not linearly separable - we need at least one hidden layer!"
    );
    
    auto data = createXORData();
    
    NeuralNetwork network({2, 4, 1});
    
    TrainingConfig config;
    config.epochs = 1000;
    config.learning_rate = 0.1;
    config.optimizer_name = "adam";
    config.loss_function = "binarycrossentropy";
    config.print_every = 100;
    
    fmt::print("\n🏗️ Network Architecture: [2, 4, 1]\n");
    fmt::print("   • Input layer: 2 neurons (x, y coordinates)\n");
    fmt::print("   • Hidden layer: 4 neurons (ReLU activation)\n");
    fmt::print("   • Output layer: 1 neuron (Sigmoid activation)\n\n");
    
    auto history = trainWithConfig(data, config, "XOR Learning");
    training_experiments_.emplace_back("XOR_Adam", history);
    
    plotLearningCurves(history, "XOR Training with Adam");
    
    fmt::print("\n🧪 Testing XOR predictions:\n");
    for (size_t i = 0; i < data.inputs.size(); ++i) {
        auto prediction = network.predict(data.inputs[i]);
        fmt::print("   ({:.0f}, {:.0f}) → {:.3f} (expected {:.0f})\n", 
                  data.inputs[i](0), data.inputs[i](1), 
                  prediction(0), data.targets[i](0));
    }
}

void TrainingTutorial::demonstrateHyperparameters()
{
    showTrainingStep("Hyperparameter Effects", 
        "Hyperparameters control how the network learns.\n"
        "Key parameters:\n"
        "• Learning rate: How big steps to take (0.001-0.1)\n"
        "• Batch size: How many examples per update (1-128)\n"
        "• Optimizer: How to update weights (SGD, Adam, etc.)\n"
        "• Network size: Number of layers and neurons"
    );
    
    auto data = createXORData();
    
    std::vector<double> learning_rates = {0.001, 0.01, 0.1};
    std::vector<QString> optimizers = {"sgd", "momentum", "adam"};
    
    for (double lr : learning_rates) {
        TrainingConfig config;
        config.epochs = 500;
        config.learning_rate = lr;
        config.optimizer_name = "adam";
        config.loss_function = "binarycrossentropy";
        config.print_every = 500;  // Only print final result
        
        NeuralNetwork network({2, 4, 1});
        auto history = trainWithConfig(data, config, 
                                     QString("Learning Rate %1").arg(lr));
        training_experiments_.emplace_back(QString("LR_%1").arg(lr), history);
    }
    
    for (const QString& opt : optimizers) {
        TrainingConfig config;
        config.epochs = 500;
        config.learning_rate = 0.01;
        config.optimizer_name = opt;
        config.loss_function = "binarycrossentropy";
        config.print_every = 500;
        
        NeuralNetwork network({2, 4, 1});
        auto history = trainWithConfig(data, config, 
                                     QString("Optimizer: %1").arg(opt));
        training_experiments_.emplace_back(QString("OPT_%1").arg(opt), history);
    }
}

void TrainingTutorial::compareOptimizers()
{
    showTrainingStep("Optimizer Comparison", 
        "Different optimizers have different convergence properties:\n"
        "• SGD: Simple, predictable, may get stuck in local minima\n"
        "• Momentum: Faster than SGD, smoother convergence\n"
        "• Adam: Adaptive learning rates, usually best choice"
    );
    
    explainTrainingMetrics();
}

void TrainingTutorial::analyzeLearningCurves()
{
    showTrainingStep("Understanding Learning Curves", 
        "Learning curves show how loss decreases during training:\n"
        "• Training loss should decrease steadily\n"
        "• Validation loss should follow training loss\n"
        "• If validation loss increases: overfitting!\n"
        "• If both plateau early: underfitting"
    );
    
    demonstrateOverfitting();
}

void TrainingTutorial::explainChemistryApplications()
{
    showTrainingStep("Chemistry Applications", 
        "Neural networks in chemistry can predict:\n"
        "• NMR model selection (classification)\n"
        "• Binding affinities (regression)\n"
        "• Reaction outcomes (classification)\n"
        "• Molecular properties (regression)\n\n"
        "SupraFit Example: Train on NMR feature vectors to predict\n"
        "which binding model (1:1, 1:2, cooperative) best fits the data."
    );
    
    if (config_.detail_level >= TutorialLevel::Advanced) {
        auto nmr_data = createClassificationData();
        
        NeuralNetwork network({10, 8, 3});  // 10 features → 3 models
        
        TrainingConfig config;
        config.epochs = 200;
        config.learning_rate = 0.001;
        config.optimizer_name = "adam";
        config.loss_function = "crossentropy";
        
        fmt::print("\n🧬 NMR Model Classification Example:\n");
        auto history = trainWithConfig(nmr_data, config, "NMR Model Selection");
        training_experiments_.emplace_back("NMR_Classification", history);
        
        plotLearningCurves(history, "NMR Model Classification");
    }
}

TrainingData TrainingTutorial::createXORData() const
{
    TrainingData data;
    
    data.inputs = {
        Eigen::Vector2d(0, 0),
        Eigen::Vector2d(0, 1),
        Eigen::Vector2d(1, 0),
        Eigen::Vector2d(1, 1)
    };
    
    data.targets = {
        Eigen::VectorXd::Zero(1),
        Eigen::VectorXd::Ones(1),
        Eigen::VectorXd::Ones(1),
        Eigen::VectorXd::Zero(1)
    };
    
    return data;
}

TrainingData TrainingTutorial::createBindingData() const
{
    TrainingData data;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<> noise(0.0, 0.1);
    
    int n_samples = 100;
    data.inputs.reserve(n_samples);
    data.targets.reserve(n_samples);
    
    for (int i = 0; i < n_samples; ++i) {
        double concentration = i / double(n_samples) * 10.0;  // 0-10 mM
        double temperature = 25.0 + (i % 20) - 10.0;         // 15-35°C
        double ph = 7.0 + (i % 7) - 3.0;                     // pH 4-10
        
        Eigen::Vector3d input(concentration, temperature, ph);
        
        double binding_affinity = 
            5.0 * std::exp(-concentration / 2.0) +  // Concentration effect
            0.1 * (temperature - 25.0) +            // Temperature effect  
            -0.5 * std::abs(ph - 7.0) +             // pH optimum at 7
            noise(gen);                              // Experimental noise
            
        Eigen::VectorXd target(1);
        target(0) = std::max(0.0, binding_affinity);
        
        data.inputs.push_back(input);
        data.targets.push_back(target);
    }
    
    return data;
}

TrainingData TrainingTutorial::createClassificationData() const
{
    TrainingData data;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<> noise(0.0, 0.05);
    
    int samples_per_class = 50;
    
    for (int model = 0; model < 3; ++model) {
        for (int i = 0; i < samples_per_class; ++i) {
            Eigen::VectorXd features(10);
            
            // Generate synthetic NMR-like features
            for (int f = 0; f < 10; ++f) {
                double base_value = 0.0;
                
                // Model-specific feature patterns
                if (model == 0) {        // 1:1 binding
                    base_value = std::sin(f * M_PI / 5.0);
                } else if (model == 1) { // 1:2 binding  
                    base_value = std::cos(f * M_PI / 3.0);
                } else {                 // Cooperative binding
                    base_value = std::sin(f * M_PI / 2.0) * std::cos(f * M_PI / 4.0);
                }
                
                features(f) = base_value + noise(gen);
            }
            
            Eigen::VectorXd target(3);
            target.setZero();
            target(model) = 1.0;  // One-hot encoding
            
            data.inputs.push_back(features);
            data.targets.push_back(target);
        }
    }
    
    return data;
}

NeuralNetwork::TrainingHistory TrainingTutorial::trainWithConfig(
    const TrainingData& data, const TrainingConfig& config, const QString& experiment_name)
{
    NeuralNetwork network({static_cast<int>(data.inputs[0].size()), 4, 
                          static_cast<int>(data.targets[0].size())});
    
    fmt::print("\n🚀 Starting experiment: {}\n", experiment_name.toStdString());
    fmt::print("   Configuration: {} optimizer, lr={}, {} epochs\n", 
              config.optimizer_name.toStdString(), config.learning_rate, config.epochs);
    
    auto history = network.train(data, config);
    
    fmt::print("   Final training loss: {:.6f}\n", history.train_loss.back());
    if (!history.validation_loss.empty()) {
        fmt::print("   Final validation loss: {:.6f}\n", history.validation_loss.back());
    }
    
    return history;
}

QJsonObject TrainingTutorial::exportTrainingResults() const
{
    QJsonObject results;
    results["tutorial_completed"] = true;
    results["experiments_run"] = static_cast<int>(training_experiments_.size());
    
    QJsonArray experiments;
    for (const auto& exp : training_experiments_) {
        QJsonObject experiment;
        experiment["name"] = exp.first;
        experiment["final_train_loss"] = exp.second.train_loss.back();
        experiment["epochs_completed"] = exp.second.epochs_completed;
        
        if (!exp.second.validation_loss.empty()) {
            experiment["final_validation_loss"] = exp.second.validation_loss.back();
        }
        
        experiments.append(experiment);
    }
    results["experiments"] = experiments;
    
    return results;
}

void TrainingTutorial::showTrainingStep(const QString& title, const QString& explanation)
{
    fmt::print("\n");
    fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::blue), "📖 {}\n", title.toStdString());
    fmt::print("{}\n", explanation.toStdString());
    
    if (config_.interactive) {
        fmt::print("\nPress Enter to continue...");
        std::cin.get();
    }
}

void TrainingTutorial::plotLearningCurves(const NeuralNetwork::TrainingHistory& history, 
                                        const QString& title)
{
    fmt::print("\n📈 Learning Curves: {}\n", title.toStdString());
    fmt::print("Epoch\tTrain Loss\tVal Loss\tTrain Acc\tVal Acc\n");
    fmt::print("-----\t----------\t--------\t---------\t-------\n");
    
    size_t n_points = std::min(size_t(10), history.train_loss.size());
    size_t step = std::max(size_t(1), history.train_loss.size() / n_points);
    
    for (size_t i = 0; i < history.train_loss.size(); i += step) {
        fmt::print("{:5}\t{:10.6f}", i + 1, history.train_loss[i]);
        
        if (i < history.validation_loss.size()) {
            fmt::print("\t{:.6f}", history.validation_loss[i]);
        } else {
            fmt::print("\t   -   ");
        }
        
        if (i < history.train_accuracy.size()) {
            fmt::print("\t  {:.3f}  ", history.train_accuracy[i]);
        } else {
            fmt::print("\t   -   ");
        }
        
        if (i < history.validation_accuracy.size()) {
            fmt::print("\t {:.3f}", history.validation_accuracy[i]);
        } else {
            fmt::print("\t  -  ");
        }
        
        fmt::print("\n");
    }
    
    // Show final values
    if (history.train_loss.size() > n_points) {
        size_t last = history.train_loss.size() - 1;
        fmt::print("{:5}\t{:10.6f}", last + 1, history.train_loss[last]);
        
        if (last < history.validation_loss.size()) {
            fmt::print("\t{:.6f}", history.validation_loss[last]);
        } else {
            fmt::print("\t   -   ");
        }
        
        if (last < history.train_accuracy.size()) {
            fmt::print("\t  {:.3f}  ", history.train_accuracy[last]);
        } else {
            fmt::print("\t   -   ");
        }
        
        if (last < history.validation_accuracy.size()) {
            fmt::print("\t {:.3f}", history.validation_accuracy[last]);
        } else {
            fmt::print("\t  -  ");
        }
        
        fmt::print("\n");
    }
}

void TrainingTutorial::compareTrainingResults()
{
    if (training_experiments_.empty()) {
        return;
    }
    
    fmt::print("\n");
    fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::magenta), 
               "📊 Training Experiment Comparison\n");
    fmt::print("===================================\n");
    
    fmt::print("Experiment\t\tFinal Loss\tEpochs\tConvergence\n");
    fmt::print("----------\t\t----------\t------\t-----------\n");
    
    for (const auto& exp : training_experiments_) {
        double final_loss = exp.second.train_loss.back();
        int epochs = exp.second.epochs_completed;
        
        QString convergence = "Poor";
        if (final_loss < 0.1) convergence = "Good";
        if (final_loss < 0.01) convergence = "Excellent";
        
        fmt::print("{:<20}\t{:10.6f}\t{:6}\t{}\n", 
                  exp.first.toStdString(), final_loss, epochs, 
                  convergence.toStdString());
    }
}

void TrainingTutorial::explainTrainingMetrics()
{
    fmt::print("\n🎯 Understanding Training Metrics:\n");
    fmt::print("• Loss: How wrong the predictions are (lower = better)\n");
    fmt::print("• Accuracy: Percentage of correct predictions (higher = better)\n");
    fmt::print("• Training vs Validation: Training = data seen, Validation = unseen data\n");
    fmt::print("• Overfitting: Training loss ↓, Validation loss ↑\n");
    fmt::print("• Underfitting: Both losses plateau at high values\n");
}

void TrainingTutorial::showDataDistribution(const TrainingData& data)
{
    fmt::print("\n📊 Dataset Statistics:\n");
    fmt::print("   Samples: {}\n", data.inputs.size());
    fmt::print("   Input dimension: {}\n", data.inputs[0].size());
    fmt::print("   Output dimension: {}\n", data.targets[0].size());
    
    if (data.targets[0].size() == 1) {
        double min_val = data.targets[0](0), max_val = data.targets[0](0);
        for (const auto& target : data.targets) {
            min_val = std::min(min_val, target(0));
            max_val = std::max(max_val, target(0));
        }
        fmt::print("   Target range: [{:.3f}, {:.3f}]\n", min_val, max_val);
    }
}

void TrainingTutorial::demonstrateOverfitting()
{
    fmt::print("\n⚠️  Overfitting Example:\n");
    fmt::print("Training a very large network on small XOR dataset...\n");
    
    auto data = createXORData();
    NeuralNetwork big_network({2, 20, 20, 1});  // Oversized for XOR
    
    TrainingConfig config;
    config.epochs = 2000;
    config.learning_rate = 0.01;
    config.optimizer_name = "adam";
    config.validation_split = 0.5;  // Very small validation set
    config.print_every = 2000;
    
    auto history = trainWithConfig(data, config, "Overfitting Demo");
    
    if (history.validation_loss.size() > 10) {
        double early_val = history.validation_loss[history.validation_loss.size() / 4];
        double late_val = history.validation_loss.back();
        
        if (late_val > early_val * 1.2) {
            fmt::print("✅ Overfitting detected: Validation loss increased from {:.6f} to {:.6f}\n", 
                      early_val, late_val);
        } else {
            fmt::print("ℹ️  No clear overfitting in this run (try running again)\n");
        }
    }
}