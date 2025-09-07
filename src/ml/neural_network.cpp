/*
 * Neural Network Core Implementation for SupraFit
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

#include "neural_network.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonArray>
#include <QtCore/QFile>
#include <QtCore/QDebug>
#include <QtCore/QElapsedTimer>
#include <fmt/format.h>
#include <algorithm>

NeuralNetwork::NeuralNetwork()
    : tutorial_manager_(nullptr)
    , last_error_(ErrorCode::Success)
{
    clearError();
}

NeuralNetwork::NeuralNetwork(const std::vector<int>& layer_sizes, 
                            const std::vector<QString>& activations)
    : NeuralNetwork()
{
    if (layer_sizes.size() < 2) {
        setError(ErrorCode::InvalidInput, "Network needs at least 2 layers (input and output)");
        return;
    }
    
    // Create layers
    for (size_t i = 0; i < layer_sizes.size() - 1; ++i) {
        QString activation = "relu";  // Default
        
        // Use provided activation if available
        if (i < activations.size() && !activations[i].isEmpty()) {
            activation = activations[i];
        }
        // Special case: use softmax for output layer in multi-class problems
        else if (i == layer_sizes.size() - 2 && layer_sizes.back() > 1) {
            activation = "softmax";
        }
        // Use linear activation for single output (regression)
        else if (i == layer_sizes.size() - 2 && layer_sizes.back() == 1) {
            activation = "linear";
        }
        
        addLayer(layer_sizes[i], layer_sizes[i + 1], activation);
    }
}

NeuralNetwork::NeuralNetwork(const NeuralNetwork& other)
    : tutorial_manager_(nullptr)  // Don't copy tutorial manager
    , metrics_(other.metrics_)
    , last_error_(other.last_error_)
    , last_error_message_(other.last_error_message_)
    , layer_outputs_(other.layer_outputs_)
    , last_prediction_(other.last_prediction_)
{
    // Deep copy layers
    layers_.reserve(other.layers_.size());
    for (const auto& layer : other.layers_) {
        layers_.push_back(std::make_unique<NeuralLayer>(*layer));
    }
}

NeuralNetwork& NeuralNetwork::operator=(const NeuralNetwork& other)
{
    if (this != &other) {
        // Clear existing layers
        layers_.clear();
        tutorial_manager_.reset();
        
        // Deep copy layers
        layers_.reserve(other.layers_.size());
        for (const auto& layer : other.layers_) {
            layers_.push_back(std::make_unique<NeuralLayer>(*layer));
        }
        
        // Copy other members
        metrics_ = other.metrics_;
        last_error_ = other.last_error_;
        last_error_message_ = other.last_error_message_;
        layer_outputs_ = other.layer_outputs_;
        last_prediction_ = other.last_prediction_;
    }
    return *this;
}

void NeuralNetwork::addLayer(int input_size, int output_size, 
                            const QString& activation,
                            NeuralLayer::InitializationMethod init)
{
    auto activationFunc = createActivationFunction(activation);
    auto layer = std::make_unique<NeuralLayer>(input_size, output_size, 
                                              std::move(activationFunc), init);
    
    // Connect tutorial manager if active
    if (tutorial_manager_) {
        layer->enableTutorialMode(tutorial_manager_.get());
    }
    
    layers_.push_back(std::move(layer));
    clearError();
}

void NeuralNetwork::clearLayers()
{
    layers_.clear();
    layer_outputs_.clear();
    clearError();
}

const NeuralLayer& NeuralNetwork::getLayer(size_t index) const
{
    if (index >= layers_.size()) {
        setError(ErrorCode::InvalidInput, QString("Layer index %1 out of range").arg(index));
        static NeuralLayer dummy(1, 1, createActivationFunction("linear"));
        return dummy;
    }
    
    return *layers_[index];
}

Eigen::VectorXd NeuralNetwork::predict(const Eigen::VectorXd& input)
{
    QElapsedTimer timer;
    timer.start();
    
    if (layers_.empty()) {
        setError(ErrorCode::EmptyNetwork, "Cannot predict with empty network");
        return Eigen::VectorXd();
    }
    
    try {
        validateInputDimensions(input);
    } catch (...) {
        return Eigen::VectorXd();  // Error already set in validate function
    }
    
    // Show input processing in tutorial mode
    if (tutorial_manager_) {
        showInputProcessing(input);
    }
    
    // Forward propagation through all layers
    Eigen::VectorXd current_input = input;
    layer_outputs_.clear();
    layer_outputs_.push_back(current_input);  // Store input
    
    for (size_t i = 0; i < layers_.size(); ++i) {
        Eigen::VectorXd layer_output = layers_[i]->forward(current_input);
        layer_outputs_.push_back(layer_output);
        
        // Show layer transition in tutorial mode
        if (tutorial_manager_) {
            showLayerTransition(i, current_input, layer_output);
        }
        
        current_input = layer_output;
    }
    
    // Show final output in tutorial mode
    if (tutorial_manager_) {
        showFinalOutput(current_input);
    }
    
    // Update performance metrics
    double elapsed_ms = timer.elapsed();
    updatePerformanceMetrics(elapsed_ms);
    
    clearError();
    return current_input;
}

NeuralNetwork::PredictionResult NeuralNetwork::predictWithDetails(const Eigen::VectorXd& input)
{
    PredictionResult result;
    result.probabilities = predict(input);
    
    if (getLastError() != ErrorCode::Success) {
        return result;  // Return empty result on error
    }
    
    // Find best prediction
    if (result.probabilities.size() > 0) {
        int max_index;
        result.confidence = result.probabilities.maxCoeff(&max_index);
        result.predicted_class = max_index;
        result.interpretation = interpretPrediction(result.probabilities);
    }
    
    last_prediction_ = result;
    return result;
}

QJsonObject NeuralNetwork::saveModel() const
{
    QJsonObject model;
    
    // Model metadata
    model["type"] = "suprafit_neural_network";
    model["version"] = "1.0";
    model["total_parameters"] = static_cast<qint64>(getTotalParameters());
    
    // Architecture
    QJsonArray architecture;
    auto arch = getArchitecture();
    for (int size : arch) {
        architecture.append(size);
    }
    model["architecture"] = architecture;
    
    // Layers
    QJsonArray layers;
    for (const auto& layer : layers_) {
        layers.append(layer->toJson());
    }
    model["layers"] = layers;
    
    // Performance metrics
    QJsonObject perf;
    perf["total_parameters"] = static_cast<qint64>(metrics_.total_parameters);
    perf["memory_usage_mb"] = metrics_.memory_usage_mb;
    model["performance_metrics"] = perf;
    
    return model;
}

bool NeuralNetwork::loadModel(const QJsonObject& json)
{
    if (json["type"].toString() != "suprafit_neural_network") {
        setError(ErrorCode::JsonError, "Invalid model type");
        return false;
    }
    
    // Clear existing network
    clearLayers();
    
    // Load layers
    QJsonArray layers = json["layers"].toArray();
    for (const auto& layerValue : layers) {
        QJsonObject layerJson = layerValue.toObject();
        
        auto layer = std::make_unique<NeuralLayer>(1, 1, createActivationFunction("linear"));
        if (!layer->fromJson(layerJson)) {
            setError(ErrorCode::JsonError, "Failed to load layer from JSON");
            clearLayers();
            return false;
        }
        
        layers_.push_back(std::move(layer));
    }
    
    // Update metrics
    if (json.contains("performance_metrics")) {
        QJsonObject perf = json["performance_metrics"].toObject();
        metrics_.total_parameters = perf["total_parameters"].toVariant().toULongLong();
        metrics_.memory_usage_mb = perf["memory_usage_mb"].toDouble();
    }
    
    clearError();
    return true;
}

bool NeuralNetwork::saveToFile(const QString& filename) const
{
    QJsonDocument doc(saveModel());
    
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        setError(ErrorCode::FileError, QString("Cannot open file for writing: %1").arg(filename));
        return false;
    }
    
    file.write(doc.toJson());
    clearError();
    return true;
}

bool NeuralNetwork::loadFromFile(const QString& filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        setError(ErrorCode::FileError, QString("Cannot open file for reading: %1").arg(filename));
        return false;
    }
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    
    if (error.error != QJsonParseError::NoError) {
        setError(ErrorCode::JsonError, QString("JSON parse error: %1").arg(error.errorString()));
        return false;
    }
    
    return loadModel(doc.object());
}

QString NeuralNetwork::getNetworkInfo() const
{
    if (layers_.empty()) {
        return "Empty network";
    }
    
    QString info = "Neural Network Architecture:\n";
    auto arch = getArchitecture();
    
    info += "  Layers: ";
    for (size_t i = 0; i < arch.size(); ++i) {
        info += QString::number(arch[i]);
        if (i < arch.size() - 1) info += " → ";
    }
    info += "\n";
    
    info += QString("  Total Parameters: %1\n").arg(getTotalParameters());
    info += QString("  Memory Usage: %1 MB\n").arg(getMemoryUsageMB(), 0, 'f', 2);
    
    return info;
}

std::vector<int> NeuralNetwork::getArchitecture() const
{
    std::vector<int> arch;
    
    if (!layers_.empty()) {
        arch.push_back(layers_[0]->inputSize());
        for (const auto& layer : layers_) {
            arch.push_back(layer->outputSize());
        }
    }
    
    return arch;
}

size_t NeuralNetwork::getTotalParameters() const
{
    size_t total = 0;
    
    for (const auto& layer : layers_) {
        // Weights + biases
        total += layer->weights().size() + layer->biases().size();
    }
    
    return total;
}

double NeuralNetwork::getMemoryUsageMB() const
{
    // Rough estimate: each parameter is a double (8 bytes)
    return (getTotalParameters() * sizeof(double)) / (1024.0 * 1024.0);
}

void NeuralNetwork::enableTutorialMode(TutorialLevel level, bool interactive)
{
    tutorial_manager_ = std::make_unique<TutorialManager>(level, interactive);
    
    // Connect tutorial manager to all layers
    for (auto& layer : layers_) {
        layer->enableTutorialMode(tutorial_manager_.get());
    }
}

void NeuralNetwork::disableTutorialMode()
{
    // Disconnect from all layers
    for (auto& layer : layers_) {
        layer->disableTutorialMode();
    }
    
    tutorial_manager_.reset();
}

void NeuralNetwork::showNetworkArchitecture() const
{
    fmt::print("\n🏗️  Network Architecture\n");
    fmt::print("============================\n");
    
    auto arch = getArchitecture();
    for (size_t i = 0; i < arch.size(); ++i) {
        if (i == 0) {
            fmt::print("Input Layer:    {} neurons\n", arch[i]);
        } else if (i == arch.size() - 1) {
            fmt::print("Output Layer:   {} neurons\n", arch[i]);
        } else {
            fmt::print("Hidden Layer {}: {} neurons\n", i, arch[i]);
        }
    }
    
    fmt::print("\nTotal Parameters: {}\n", getTotalParameters());
    fmt::print("Memory Usage: {:.2f} MB\n", getMemoryUsageMB());
}

bool NeuralNetwork::validateArchitecture() const
{
    if (layers_.empty()) {
        setError(ErrorCode::EmptyNetwork, "Network has no layers");
        return false;
    }
    
    // Check layer compatibility
    for (size_t i = 1; i < layers_.size(); ++i) {
        if (layers_[i-1]->outputSize() != layers_[i]->inputSize()) {
            setError(ErrorCode::DimensionMismatch, 
                    QString("Layer dimension mismatch between layer %1 and %2").arg(i-1).arg(i));
            return false;
        }
    }
    
    clearError();
    return true;
}

void NeuralNetwork::updatePerformanceMetrics(double inference_time_ms)
{
    metrics_.inference_time_ms = inference_time_ms;
    metrics_.total_parameters = getTotalParameters();
    metrics_.memory_usage_mb = getMemoryUsageMB();
}

void NeuralNetwork::validateInputDimensions(const Eigen::VectorXd& input) const
{
    if (layers_.empty()) {
        setError(ErrorCode::EmptyNetwork, "Cannot validate input for empty network");
        throw std::runtime_error("Empty network");
    }
    
    if (input.size() != layers_[0]->inputSize()) {
        setError(ErrorCode::DimensionMismatch, 
                QString("Input dimension mismatch. Expected: %1, Got: %2")
                .arg(layers_[0]->inputSize()).arg(input.size()));
        throw std::runtime_error("Dimension mismatch");
    }
}

void NeuralNetwork::setError(ErrorCode code, const QString& message) const
{
    last_error_ = code;
    last_error_message_ = message;
}

void NeuralNetwork::clearError() const
{
    last_error_ = ErrorCode::Success;
    last_error_message_.clear();
}

void NeuralNetwork::showInputProcessing(const Eigen::VectorXd& input) const
{
    if (!tutorial_manager_) return;
    
    tutorial_manager_->showStep("Processing network input");
    tutorial_manager_->showVector("Input Features", input);
    tutorial_manager_->explainConcept("Forward Propagation", 
        "The input will now be passed through each layer of the network. "
        "Each layer performs: output = activation(weights × input + biases)");
}

void NeuralNetwork::showLayerTransition(size_t layer_index, 
                                      const Eigen::VectorXd& input,
                                      const Eigen::VectorXd& output) const
{
    if (!tutorial_manager_) return;
    
    tutorial_manager_->showStep(QString("Layer %1: %2").arg(layer_index + 1)
                               .arg(layers_[layer_index]->getLayerInfo()));
    
    if (tutorial_manager_->level_ != TutorialLevel::Basic) {
        tutorial_manager_->showVector("Layer Input", input);
        tutorial_manager_->showMatrix("Weights", layers_[layer_index]->weights());
        tutorial_manager_->showVector("Biases", layers_[layer_index]->biases());
    }
    
    tutorial_manager_->showVector("Layer Output", output);
}

void NeuralNetwork::showFinalOutput(const Eigen::VectorXd& output) const
{
    if (!tutorial_manager_) return;
    
    tutorial_manager_->showStep("Final Network Output");
    tutorial_manager_->showVector("Output", output);
    tutorial_manager_->explainConcept("Interpretation", interpretPrediction(output));
}

QString NeuralNetwork::interpretPrediction(const Eigen::VectorXd& output) const
{
    if (output.size() == 1) {
        return QString("Single output value: %1 (regression or binary classification)")
               .arg(output(0), 0, 'f', 6);
    } else {
        // Multi-class classification
        int max_index;
        double max_value = output.maxCoeff(&max_index);
        
        return QString("Multi-class prediction: Class %1 with value %2 (confidence: %3%)")
               .arg(max_index)
               .arg(max_value, 0, 'f', 6)
               .arg(max_value * 100, 0, 'f', 2);
    }
}

void NeuralNetwork::setLayerWeights(size_t layer_index, const Eigen::MatrixXd& weights)
{
    if (layer_index >= layers_.size()) {
        setError(ErrorCode::InvalidInput, QString("Layer index %1 out of range").arg(layer_index));
        return;
    }
    
    layers_[layer_index]->setWeights(weights);
    clearError();
}

void NeuralNetwork::setLayerBiases(size_t layer_index, const Eigen::VectorXd& biases)
{
    if (layer_index >= layers_.size()) {
        setError(ErrorCode::InvalidInput, QString("Layer index %1 out of range").arg(layer_index));
        return;
    }
    
    layers_[layer_index]->setBiases(biases);
    clearError();
}

void NeuralNetwork::showLayerDetails(size_t layer_index) const
{
    if (layer_index >= layers_.size()) {
        fmt::print("❌ Layer index {} out of range\n", layer_index);
        return;
    }
    
    fmt::print("\n🔍 Layer {} Details:\n", layer_index + 1);
    fmt::print("===================\n");
    fmt::print("{}\n", layers_[layer_index]->getLayerInfo().toStdString());
    
    if (tutorial_manager_) {
        tutorial_manager_->showMatrix("Weights", layers_[layer_index]->weights());
        tutorial_manager_->showVector("Biases", layers_[layer_index]->biases());
    }
}

void NeuralNetwork::showLastPredictionDetails() const
{
    if (last_prediction_.probabilities.size() == 0) {
        fmt::print("❌ No previous prediction available\n");
        return;
    }
    
    fmt::print("\n📊 Last Prediction Details:\n");
    fmt::print("===========================\n");
    fmt::print("Predicted Class: {}\n", last_prediction_.predicted_class);
    fmt::print("Confidence: {:.6f}\n", last_prediction_.confidence);
    fmt::print("Interpretation: {}\n", last_prediction_.interpretation.toStdString());
    
    if (tutorial_manager_) {
        tutorial_manager_->showVector("Output Probabilities", last_prediction_.probabilities);
    }
}

void NeuralNetwork::printNetworkSummary() const
{
    fmt::print("\n📋 Network Summary:\n");
    fmt::print("==================\n");
    fmt::print("{}", getNetworkInfo().toStdString());
}

void NeuralNetwork::printWeightsAndBiases() const
{
    fmt::print("\n⚖️  Network Weights and Biases:\n");
    fmt::print("==============================\n");
    
    for (size_t i = 0; i < layers_.size(); ++i) {
        fmt::print("\nLayer {} ({}):\n", i + 1, layers_[i]->getLayerInfo().toStdString());
        
        const auto& weights = layers_[i]->weights();
        const auto& biases = layers_[i]->biases();
        
        fmt::print("Weights ({} x {}):\n", weights.rows(), weights.cols());
        for (int row = 0; row < weights.rows(); ++row) {
            fmt::print("  [");
            for (int col = 0; col < weights.cols(); ++col) {
                fmt::print(" {:.6f}", weights(row, col));
            }
            fmt::print(" ]\n");
        }
        
        fmt::print("Biases ({}):\n", biases.size());
        fmt::print("  [");
        for (int i = 0; i < biases.size(); ++i) {
            fmt::print(" {:.6f}", biases(i));
        }
        fmt::print(" ]\n");
    }
}

// ================================================================================
// NEURAL NETWORK TRAINING IMPLEMENTATION (Claude Generated - 2025)
// ================================================================================

NeuralNetwork::TrainingHistory NeuralNetwork::train(const TrainingData& training_data, const TrainingConfig& config)
{
    TrainingHistory history;
    
    if (training_data.size() == 0) {
        setError(ErrorCode::InvalidInput, "Training data is empty");
        return history;
    }
    
    // Create optimizer and loss function
    auto optimizer = createOptimizer(config.optimizer_name, config.learning_rate);
    auto loss_function = createLossFunction(config.loss_function);
    
    // Split data into training and validation sets
    size_t total_size = training_data.size();
    size_t validation_size = static_cast<size_t>(total_size * config.validation_split);
    size_t train_size = total_size - validation_size;
    
    TrainingData train_set, validation_set;
    for (size_t i = 0; i < train_size; ++i) {
        train_set.inputs.push_back(training_data.inputs[i]);
        train_set.targets.push_back(training_data.targets[i]);
    }
    for (size_t i = train_size; i < total_size; ++i) {
        validation_set.inputs.push_back(training_data.inputs[i]);
        validation_set.targets.push_back(training_data.targets[i]);
    }
    
    fmt::print("\n🎯 Training Neural Network\n");
    fmt::print("==========================\n");
    fmt::print("Training samples: {}\n", train_set.size());
    fmt::print("Validation samples: {}\n", validation_set.size());
    fmt::print("Epochs: {}, Batch size: {}\n", config.epochs, config.batch_size);
    fmt::print("Optimizer: {}, Loss: {}\n", config.optimizer_name.toStdString(), config.loss_function.toStdString());
    fmt::print("\n");
    
    // Training loop
    for (int epoch = 0; epoch < config.epochs; ++epoch) {
        double total_train_loss = 0.0;
        int batch_count = 0;
        
        // Create mini-batches
        auto batches = train_set.createMiniBatches(config.batch_size);
        
        for (const auto& batch : batches) {
            const auto& batch_inputs = batch.first;
            const auto& batch_targets = batch.second;
            
            double batch_loss = 0.0;
            
            // Process each sample in the batch
            for (size_t i = 0; i < batch_inputs.size(); ++i) {
                batch_loss += backpropagate(batch_inputs[i], batch_targets[i], *loss_function);
            }
            
            batch_loss /= batch_inputs.size();
            total_train_loss += batch_loss;
            batch_count++;
        }
        
        total_train_loss /= batch_count;
        
        // Calculate validation loss
        double validation_loss = 0.0;
        if (validation_set.size() > 0) {
            for (size_t i = 0; i < validation_set.size(); ++i) {
                Eigen::VectorXd pred = predict(validation_set.inputs[i]);
                validation_loss += loss_function->forward(pred, validation_set.targets[i]);
            }
            validation_loss /= validation_set.size();
        }
        
        // Calculate accuracies
        double train_accuracy = calculateAccuracy(train_set);
        double validation_accuracy = validation_set.size() > 0 ? calculateAccuracy(validation_set) : 0.0;
        
        // Store history
        history.train_loss.push_back(total_train_loss);
        history.validation_loss.push_back(validation_loss);
        history.train_accuracy.push_back(train_accuracy);
        history.validation_accuracy.push_back(validation_accuracy);
        history.epochs_completed = epoch + 1;
        
        // Print progress
        if ((epoch + 1) % config.print_every == 0 || epoch == 0 || epoch == config.epochs - 1) {
            fmt::print("Epoch {:3d}/{}: Loss: {:.6f}, Val Loss: {:.6f}, Acc: {:.3f}, Val Acc: {:.3f}\n",
                      epoch + 1, config.epochs, total_train_loss, validation_loss, 
                      train_accuracy, validation_accuracy);
        }
    }
    
    fmt::print("\n✅ Training completed!\n");
    fmt::print("Final training accuracy: {:.3f}\n", history.train_accuracy.back());
    fmt::print("Final validation accuracy: {:.3f}\n", history.validation_accuracy.back());
    
    return history;
}

double NeuralNetwork::backpropagate(const Eigen::VectorXd& input, const Eigen::VectorXd& target, 
                                   const LossFunction& loss_function)
{
    if (layers_.empty()) {
        setError(ErrorCode::InvalidState, "No layers in network for backpropagation");
        return 0.0;
    }
    
    // Forward pass - store intermediate results
    std::vector<Eigen::VectorXd> layer_inputs;
    std::vector<Eigen::VectorXd> layer_outputs;
    
    Eigen::VectorXd current_input = input;
    layer_inputs.push_back(current_input);
    
    for (auto& layer : layers_) {
        current_input = layer->forward(current_input);
        layer_outputs.push_back(current_input);
        if (layer != layers_.back()) {  // Don't add output of last layer as input to next
            layer_inputs.push_back(current_input);
        }
    }
    
    // Calculate loss
    Eigen::VectorXd predictions = layer_outputs.back();
    double loss = loss_function.forward(predictions, target);
    
    // Backward pass - calculate gradients
    Eigen::VectorXd current_gradient = loss_function.backward(predictions, target);
    
    // Backpropagate through layers
    for (int i = layers_.size() - 1; i >= 0; --i) {
        auto& layer = layers_[i];
        Eigen::VectorXd layer_input = layer_inputs[i];
        
        // Calculate gradients for this layer
        Eigen::MatrixXd weight_gradients;
        Eigen::VectorXd bias_gradients;
        Eigen::VectorXd input_gradient;
        
        layer->backward(layer_input, current_gradient, weight_gradients, bias_gradients, input_gradient);
        
        // Update weights using stored optimizer (we'll need to add this to the layer)
        // For now, use simple SGD update
        double learning_rate = 0.01;  // This should come from optimizer
        Eigen::MatrixXd weights = layer->getWeights();
        Eigen::VectorXd biases = layer->getBiases();
        
        weights -= learning_rate * weight_gradients;
        biases -= learning_rate * bias_gradients;
        
        layer->setWeights(weights);
        layer->setBiases(biases);
        
        // Prepare gradient for next layer
        current_gradient = input_gradient;
    }
    
    return loss;
}

double NeuralNetwork::calculateAccuracy(const TrainingData& data, double threshold) const
{
    if (data.size() == 0) return 0.0;
    
    int correct = 0;
    for (size_t i = 0; i < data.size(); ++i) {
        Eigen::VectorXd prediction = const_cast<NeuralNetwork*>(this)->predict(data.inputs[i]);
        Eigen::VectorXd target = data.targets[i];
        
        // For binary classification
        if (prediction.size() == 1 && target.size() == 1) {
            bool pred_class = prediction(0) >= threshold;
            bool true_class = target(0) >= threshold;
            if (pred_class == true_class) correct++;
        }
        // For multi-class classification
        else {
            int pred_class = 0, true_class = 0;
            prediction.maxCoeff(&pred_class);
            target.maxCoeff(&true_class);
            if (pred_class == true_class) correct++;
        }
    }
    
    return static_cast<double>(correct) / data.size();
}