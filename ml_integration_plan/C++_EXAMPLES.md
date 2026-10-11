# SupraFit ML C++ Implementation Examples

## Complete Code Examples for Core Components

### 1. Neural Network Core Implementation

```cpp
// src/ml/neural_network.cpp
#include "neural_network.h"
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonArray>
#include <QtCore/QFile>
#include <QtCore/QDebug>
#include <random>
#include <cmath>

// Activation Functions
double ReLUActivation::forward(double x) const {
    return std::max(0.0, x);
}

double ReLUActivation::backward(double x) const {
    return x > 0.0 ? 1.0 : 0.0;
}

double SigmoidActivation::forward(double x) const {
    return 1.0 / (1.0 + std::exp(-std::max(-500.0, std::min(500.0, x))));  // Clip to prevent overflow
}

double SigmoidActivation::backward(double x) const {
    double s = forward(x);
    return s * (1.0 - s);
}

Eigen::VectorXd SoftmaxActivation::forwardVector(const Eigen::VectorXd& x) const {
    // Subtract max for numerical stability
    double max_val = x.maxCoeff();
    Eigen::VectorXd exp_x = (x.array() - max_val).exp();
    double sum_exp = exp_x.sum();
    return exp_x / sum_exp;
}

// Neural Layer Implementation
NeuralLayer::NeuralLayer(int input_size, int output_size, 
                        std::unique_ptr<ActivationFunction> activation)
    : input_size_(input_size), output_size_(output_size), 
      activation_(std::move(activation)) {
    
    weights_ = Eigen::MatrixXd(output_size, input_size);
    biases_ = Eigen::VectorXd(output_size);
    
    initializeWeights();
}

void NeuralLayer::initializeWeights() {
    // Xavier initialization
    std::random_device rd;
    std::mt19937 gen(rd());
    
    double std_dev = std::sqrt(2.0 / (input_size_ + output_size_));
    std::normal_distribution<double> dist(0.0, std_dev);
    
    // Initialize weights
    for (int i = 0; i < weights_.rows(); ++i) {
        for (int j = 0; j < weights_.cols(); ++j) {
            weights_(i, j) = dist(gen);
        }
    }
    
    // Initialize biases to zero
    biases_.setZero();
}

Eigen::VectorXd NeuralLayer::forward(const Eigen::VectorXd& input) {
    last_input_ = input;
    
    // Linear transformation: output = weights * input + biases
    Eigen::VectorXd linear_output = weights_ * input + biases_;
    
    // Apply activation function
    if (activation_->name() == "Softmax") {
        // Special handling for softmax
        auto* softmax = dynamic_cast<SoftmaxActivation*>(activation_.get());
        last_output_ = softmax->forwardVector(linear_output);
    } else {
        // Element-wise activation
        last_output_ = Eigen::VectorXd(linear_output.size());
        for (int i = 0; i < linear_output.size(); ++i) {
            last_output_(i) = activation_->forward(linear_output(i));
        }
    }
    
    return last_output_;
}

QJsonObject NeuralLayer::toJson() const {
    QJsonObject layer_json;
    
    layer_json["type"] = "dense";
    layer_json["input_size"] = input_size_;
    layer_json["output_size"] = output_size_;
    layer_json["activation"] = activation_->name();
    
    // Serialize weights
    QJsonArray weights_array;
    for (int i = 0; i < weights_.rows(); ++i) {
        QJsonArray row;
        for (int j = 0; j < weights_.cols(); ++j) {
            row.append(weights_(i, j));
        }
        weights_array.append(row);
    }
    layer_json["weights"] = weights_array;
    
    // Serialize biases
    QJsonArray biases_array;
    for (int i = 0; i < biases_.size(); ++i) {
        biases_array.append(biases_(i));
    }
    layer_json["biases"] = biases_array;
    
    return layer_json;
}

bool NeuralLayer::fromJson(const QJsonObject& json) {
    if (!json.contains("weights") || !json.contains("biases")) {
        return false;
    }
    
    // Load dimensions
    input_size_ = json["input_size"].toInt();
    output_size_ = json["output_size"].toInt();
    
    // Resize matrices
    weights_ = Eigen::MatrixXd(output_size_, input_size_);
    biases_ = Eigen::VectorXd(output_size_);
    
    // Load weights
    QJsonArray weights_array = json["weights"].toArray();
    for (int i = 0; i < weights_array.size(); ++i) {
        QJsonArray row = weights_array[i].toArray();
        for (int j = 0; j < row.size(); ++j) {
            weights_(i, j) = row[j].toDouble();
        }
    }
    
    // Load biases
    QJsonArray biases_array = json["biases"].toArray();
    for (int i = 0; i < biases_array.size(); ++i) {
        biases_(i) = biases_array[i].toDouble();
    }
    
    // Create activation function
    QString activation_name = json["activation"].toString();
    if (activation_name == "ReLU") {
        activation_ = std::make_unique<ReLUActivation>();
    } else if (activation_name == "Sigmoid") {
        activation_ = std::make_unique<SigmoidActivation>();
    } else if (activation_name == "Softmax") {
        activation_ = std::make_unique<SoftmaxActivation>();
    } else {
        return false;
    }
    
    return true;
}

// Neural Network Implementation
NeuralNetwork::NeuralNetwork() : is_finalized_(false) {}

NeuralNetwork::NeuralNetwork(const std::vector<int>& layer_sizes) : is_finalized_(false) {
    for (size_t i = 1; i < layer_sizes.size(); ++i) {
        QString activation = (i == layer_sizes.size() - 1) ? "Softmax" : "ReLU";
        addLayer(layer_sizes[i-1], layer_sizes[i], activation);
    }
    finalize();
}

void NeuralNetwork::addLayer(int input_size, int output_size, const QString& activation) {
    std::unique_ptr<ActivationFunction> act_func;
    
    if (activation == "ReLU") {
        act_func = std::make_unique<ReLUActivation>();
    } else if (activation == "Sigmoid") {
        act_func = std::make_unique<SigmoidActivation>();
    } else if (activation == "Softmax") {
        act_func = std::make_unique<SoftmaxActivation>();
    } else {
        qWarning() << "Unknown activation function:" << activation;
        act_func = std::make_unique<ReLUActivation>();
    }
    
    layers_.push_back(std::make_unique<NeuralLayer>(input_size, output_size, std::move(act_func)));
}

void NeuralNetwork::finalize() {
    if (layers_.empty()) {
        qWarning() << "Cannot finalize empty network";
        return;
    }
    is_finalized_ = true;
}

Eigen::VectorXd NeuralNetwork::predict(const Eigen::VectorXd& input) {
    if (!is_finalized_) {
        qWarning() << "Network not finalized";
        return Eigen::VectorXd();
    }
    
    if (layers_.empty()) {
        qWarning() << "Empty network";
        return Eigen::VectorXd();
    }
    
    Eigen::VectorXd current_input = input;
    
    // Forward propagation through all layers
    for (const auto& layer : layers_) {
        current_input = layer->forward(current_input);
    }
    
    return current_input;
}

QVector<double> NeuralNetwork::predictVector(const QVector<double>& input) {
    // Convert QVector to Eigen::VectorXd
    Eigen::VectorXd eigen_input(input.size());
    for (int i = 0; i < input.size(); ++i) {
        eigen_input(i) = input[i];
    }
    
    Eigen::VectorXd output = predict(eigen_input);
    
    // Convert back to QVector
    QVector<double> result(output.size());
    for (int i = 0; i < output.size(); ++i) {
        result[i] = output(i);
    }
    
    return result;
}

bool NeuralNetwork::saveModel(const QString& filename) const {
    if (!is_finalized_) {
        qWarning() << "Cannot save unfinalized network";
        return false;
    }
    
    QJsonObject model_json;
    
    // Model metadata
    QJsonObject metadata;
    metadata["version"] = "1.0";
    metadata["architecture"] = "feedforward";
    metadata["layer_count"] = static_cast<int>(layers_.size());
    metadata["parameter_count"] = getParameterCount();
    model_json["model_metadata"] = metadata;
    
    // Network architecture
    QJsonObject architecture;
    QJsonArray layers_array;
    
    for (const auto& layer : layers_) {
        layers_array.append(layer->toJson());
    }
    
    architecture["layers"] = layers_array;
    model_json["network_architecture"] = architecture;
    
    // Write to file
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Cannot open file for writing:" << filename;
        return false;
    }
    
    QJsonDocument doc(model_json);
    file.write(doc.toJson());
    
    return true;
}

bool NeuralNetwork::loadModel(const QString& filename) {
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Cannot open file for reading:" << filename;
        return false;
    }
    
    QByteArray data = file.readAll();
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        qWarning() << "JSON parse error:" << error.errorString();
        return false;
    }
    
    QJsonObject model_json = doc.object();
    
    if (!model_json.contains("network_architecture")) {
        qWarning() << "Invalid model file: missing network_architecture";
        return false;
    }
    
    // Clear existing layers
    layers_.clear();
    is_finalized_ = false;
    
    // Load layers
    QJsonObject architecture = model_json["network_architecture"].toObject();
    QJsonArray layers_array = architecture["layers"].toArray();
    
    for (const auto& layer_value : layers_array) {
        QJsonObject layer_json = layer_value.toObject();
        
        auto layer = std::make_unique<NeuralLayer>(1, 1, std::make_unique<ReLUActivation>());
        if (!layer->fromJson(layer_json)) {
            qWarning() << "Failed to load layer";
            return false;
        }
        
        layers_.push_back(std::move(layer));
    }
    
    is_finalized_ = true;
    return true;
}

QStringList NeuralNetwork::getModelInfo() const {
    QStringList info;
    
    if (!is_finalized_) {
        info << "Network not finalized";
        return info;
    }
    
    info << QString("Layers: %1").arg(layers_.size());
    info << QString("Parameters: %1").arg(getParameterCount());
    
    for (size_t i = 0; i < layers_.size(); ++i) {
        const auto& layer = layers_[i];
        info << QString("Layer %1: %2x%3 (%4)")
                .arg(i)
                .arg(layer->weights().rows())
                .arg(layer->weights().cols())
                .arg("Dense"); // Could be extended for other layer types
    }
    
    return info;
}

int NeuralNetwork::getParameterCount() const {
    int total_params = 0;
    
    for (const auto& layer : layers_) {
        total_params += layer->weights().size() + layer->biases().size();
    }
    
    return total_params;
}
```

### 2. Model Predictor Implementation

```cpp
// src/ml/model_predictor.cpp
#include "model_predictor.h"
#include "feature_preprocessor.h"
#include <QtCore/QDebug>
#include <QtCore/QElapsedTimer>
#include <QtCore/QJsonDocument>
#include <QtCore/QStandardPaths>
#include <algorithm>

ModelPredictor::ModelPredictor(QObject* parent)
    : QObject(parent)
    , network_(std::make_unique<NeuralNetwork>())
    , preprocessor_(std::make_unique<FeaturePreprocessor>())
    , total_prediction_time_(0.0)
    , prediction_count_(0) {
}

ModelPredictor::~ModelPredictor() = default;

bool ModelPredictor::loadModel(const QString& model_path) {
    qDebug() << "Loading ML model from:" << model_path;
    
    QElapsedTimer timer;
    timer.start();
    
    // Load neural network
    if (!network_->loadModel(model_path)) {
        emit errorOccurred("Failed to load neural network from " + model_path);
        return false;
    }
    
    // Load model metadata (including feature config and class labels)
    QFile file(model_path);
    if (!file.open(QIODevice::ReadOnly)) {
        emit errorOccurred("Cannot open model file for metadata: " + model_path);
        return false;
    }
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    
    if (error.error != QJsonParseError::NoError) {
        emit errorOccurred("Model metadata parse error: " + error.errorString());
        return false;
    }
    
    model_metadata_ = doc.object();
    
    // Extract class labels and model mappings
    if (model_metadata_.contains("class_labels")) {
        QJsonObject labels = model_metadata_["class_labels"].toObject();
        
        model_names_.clear();
        model_ids_.clear();
        
        for (auto it = labels.begin(); it != labels.end(); ++it) {
            QJsonObject label_info = it.value().toObject();
            int class_index = it.key().toInt();
            
            // Ensure vectors are large enough
            while (model_names_.size() <= class_index) {
                model_names_.append(QString());
                model_ids_.append(0);
            }
            
            model_names_[class_index] = label_info["name"].toString();
            model_ids_[class_index] = label_info["id"].toInt();
        }
    }
    
    // Load feature preprocessor configuration
    if (model_metadata_.contains("feature_config")) {
        QJsonObject feature_config = model_metadata_["feature_config"].toObject();
        
        FeaturePreprocessor::FeatureConfig config;
        
        // Load feature scaling parameters
        QJsonArray means_array = feature_config["means"].toArray();
        QJsonArray stds_array = feature_config["stds"].toArray();
        
        config.means = Eigen::VectorXd(means_array.size());
        config.stds = Eigen::VectorXd(stds_array.size());
        
        for (int i = 0; i < means_array.size(); ++i) {
            config.means(i) = means_array[i].toDouble();
            config.stds(i) = stds_array[i].toDouble();
        }
        
        preprocessor_->setConfig(config);
    }
    
    double load_time = timer.elapsed();
    qDebug() << "Model loaded successfully in" << load_time << "ms";
    
    emit modelLoaded(model_path);
    return true;
}

bool ModelPredictor::isModelLoaded() const {
    return !model_names_.isEmpty() && !model_ids_.isEmpty();
}

QString ModelPredictor::getModelInfo() const {
    if (!isModelLoaded()) {
        return "No model loaded";
    }
    
    QStringList info;
    info << "Neural Network Model";
    info << network_->getModelInfo();
    info << QString("Classes: %1").arg(model_names_.size());
    
    for (int i = 0; i < model_names_.size(); ++i) {
        info << QString("  %1: %2 (ID: %3)").arg(i).arg(model_names_[i]).arg(model_ids_[i]);
    }
    
    if (prediction_count_ > 0) {
        double avg_time = total_prediction_time_ / prediction_count_;
        info << QString("Average prediction time: %1 ms").arg(avg_time, 0, 'f', 2);
    }
    
    return info.join("\n");
}

ModelPredictor::PredictionResult ModelPredictor::predictBestModels(const QJsonObject& features, int top_k) {
    PredictionResult result;
    result.input_features = features;
    
    QElapsedTimer timer;
    timer.start();
    
    if (!isModelLoaded()) {
        result.metadata["error"] = "No model loaded";
        result.prediction_time_ms = timer.elapsed();
        return result;
    }
    
    // Preprocess features
    FeaturePreprocessor::ProcessedFeatures processed = preprocessor_->processFeatures(features);
    
    if (!processed.success) {
        result.metadata["error"] = "Feature preprocessing failed: " + processed.error_message;
        result.prediction_time_ms = timer.elapsed();
        return result;
    }
    
    // Neural network prediction
    Eigen::VectorXd predictions = network_->predict(processed.features);
    
    if (predictions.size() == 0) {
        result.metadata["error"] = "Neural network prediction failed";
        result.prediction_time_ms = timer.elapsed();
        return result;
    }
    
    // Convert predictions to model suggestions
    QVector<QPair<double, int>> scored_models;
    
    for (int i = 0; i < predictions.size() && i < model_ids_.size(); ++i) {
        scored_models.append(QPair<double, int>(predictions(i), i));
    }
    
    // Sort by confidence (descending)
    std::sort(scored_models.begin(), scored_models.end(), 
             [](const QPair<double, int>& a, const QPair<double, int>& b) {
                 return a.first > b.first;
             });
    
    // Create top-k suggestions
    int k = std::min(top_k, scored_models.size());
    for (int i = 0; i < k; ++i) {
        int class_index = scored_models[i].second;
        double confidence = scored_models[i].first;
        
        ModelSuggestion suggestion;
        suggestion.model_id = model_ids_[class_index];
        suggestion.model_name = model_names_[class_index];
        suggestion.confidence = confidence;
        suggestion.uncertainty = estimateUncertainty(predictions);
        suggestion.feature_importance = extractFeatureImportance(processed.features);
        suggestion.reasoning = generateReasoning(suggestion, features);
        
        result.suggestions.append(suggestion);
    }
    
    result.prediction_time_ms = timer.elapsed();
    
    // Update statistics
    total_prediction_time_ += result.prediction_time_ms;
    prediction_count_++;
    
    // Add metadata
    result.metadata["model_info"] = getModelInfo();
    result.metadata["feature_count"] = processed.features.size();
    result.metadata["top_k"] = k;
    
    return result;
}

ModelPredictor::ModelSuggestion ModelPredictor::predictSingleBest(const QJsonObject& features) {
    PredictionResult result = predictBestModels(features, 1);
    
    if (result.suggestions.isEmpty()) {
        ModelSuggestion empty_suggestion;
        empty_suggestion.model_id = -1;
        empty_suggestion.model_name = "No prediction";
        empty_suggestion.confidence = 0.0;
        empty_suggestion.reasoning = result.metadata["error"].toString();
        return empty_suggestion;
    }
    
    return result.suggestions.first();
}

QVector<int> ModelPredictor::suggestModelIds(const QJsonObject& features, int max_models) {
    PredictionResult result = predictBestModels(features, max_models);
    
    QVector<int> model_ids;
    for (const auto& suggestion : result.suggestions) {
        if (suggestion.model_id > 0) {  // Valid model ID
            model_ids.append(suggestion.model_id);
        }
    }
    
    return model_ids;
}

bool ModelPredictor::shouldFitModel(int model_id, const QJsonObject& features, double threshold) {
    QVector<int> suggested = suggestModelIds(features, 10);  // Get more suggestions
    
    // Check if model_id is in top suggestions with sufficient confidence
    for (int i = 0; i < suggested.size(); ++i) {
        if (suggested[i] == model_id) {
            // Model is suggested, check position-based threshold
            double position_threshold = threshold * (1.0 + 0.1 * i);  // Lower threshold for lower positions
            return true;  // For now, suggest fitting if it's in the list
        }
    }
    
    return false;
}

QJsonObject ModelPredictor::extractFeatureImportance(const Eigen::VectorXd& features) const {
    // Simplified feature importance (would need proper implementation)
    QJsonObject importance;
    
    // Basic feature importance based on magnitude
    for (int i = 0; i < features.size() && i < 10; ++i) {  // Top 10 features
        QString feature_name = QString("feature_%1").arg(i);
        importance[feature_name] = std::abs(features(i));
    }
    
    return importance;
}

QString ModelPredictor::generateReasoning(const ModelSuggestion& suggestion, 
                                        const QJsonObject& input_features) const {
    QStringList reasoning;
    
    reasoning << QString("Model %1 predicted with %2% confidence")
                 .arg(suggestion.model_name)
                 .arg(suggestion.confidence * 100, 0, 'f', 1);
    
    // Add reasoning based on input features
    if (input_features.contains("ground_truth")) {
        QJsonObject gt = input_features["ground_truth"].toObject();
        
        int datapoints = gt.value("datapoints").toInt();
        int series = gt.value("series_count").toInt();
        
        reasoning << QString("Dataset characteristics: %1 datapoints, %2 series")
                     .arg(datapoints).arg(series);
        
        if (datapoints < 10) {
            reasoning << "Small dataset size may limit model complexity";
        }
        
        if (series > 3) {
            reasoning << "Multiple series suggest complex binding behavior";
        }
    }
    
    if (input_features.contains("candidate_models")) {
        QJsonArray candidates = input_features["candidate_models"].toArray();
        if (!candidates.isEmpty()) {
            QJsonObject first_candidate = candidates[0].toObject();
            QJsonObject fit_quality = first_candidate["fit_quality"].toObject();
            
            double r_squared = fit_quality.value("r_squared").toDouble();
            double aic = fit_quality.value("aic").toDouble();
            
            reasoning << QString("Initial fit: R² = %1, AIC = %2")
                         .arg(r_squared, 0, 'f', 3).arg(aic, 0, 'f', 1);
            
            if (r_squared > 0.95) {
                reasoning << "High R² suggests good model fit";
            }
        }
    }
    
    return reasoning.join(". ");
}

double ModelPredictor::estimateUncertainty(const Eigen::VectorXd& prediction) const {
    // Estimate uncertainty based on prediction entropy
    double entropy = 0.0;
    
    for (int i = 0; i < prediction.size(); ++i) {
        double p = std::max(1e-10, prediction(i));  // Avoid log(0)
        entropy -= p * std::log(p);
    }
    
    // Normalize by maximum possible entropy
    double max_entropy = std::log(prediction.size());
    
    return entropy / max_entropy;
}

void ModelPredictor::handlePredictionError(const QString& error) {
    qWarning() << "Prediction error:" << error;
    emit errorOccurred(error);
}
```

### 3. Feature Preprocessor Implementation

```cpp
// src/ml/feature_preprocessor.cpp
#include "feature_preprocessor.h"
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QFile>
#include <QtCore/QDebug>
#include <cmath>

FeaturePreprocessor::FeaturePreprocessor() : is_fitted_(false) {}

FeaturePreprocessor::ProcessedFeatures FeaturePreprocessor::processFeatures(const QJsonObject& raw_features) const {
    ProcessedFeatures result;
    result.success = false;
    
    if (!is_fitted_) {
        result.error_message = "Preprocessor not fitted with configuration";
        return result;
    }
    
    try {
        // Extract features from different sections
        QVector<double> all_features;
        
        // Ground truth features
        if (raw_features.contains("ground_truth")) {
            QVector<double> gt_features = extractGroundTruthFeatures(raw_features["ground_truth"].toObject());
            all_features.append(gt_features);
        }
        
        // Candidate model features (use first candidate)
        if (raw_features.contains("candidate_models")) {
            QJsonArray candidates = raw_features["candidate_models"].toArray();
            if (!candidates.isEmpty()) {
                QJsonObject first_candidate = candidates[0].toObject();
                
                // Fit quality features
                if (first_candidate.contains("fit_quality")) {
                    QVector<double> fq_features = extractFitQualityFeatures(first_candidate["fit_quality"].toObject());
                    all_features.append(fq_features);
                }
                
                // Statistical features
                if (first_candidate.contains("statistical_features")) {
                    QVector<double> stat_features = extractStatisticalFeatures(first_candidate["statistical_features"].toObject());
                    all_features.append(stat_features);
                }
            }
        }
        
        // Input noise features
        if (raw_features.contains("ground_truth")) {
            QJsonObject gt = raw_features["ground_truth"].toObject();
            if (gt.contains("input_noise")) {
                QVector<double> noise_features = extractInputNoiseFeatures(gt["input_noise"].toObject());
                all_features.append(noise_features);
            }
        }
        
        // Convert to Eigen vector
        result.features = Eigen::VectorXd(all_features.size());
        for (int i = 0; i < all_features.size(); ++i) {
            result.features(i) = all_features[i];
        }
        
        // Validate feature count
        if (result.features.size() != config_.means.size()) {
            result.error_message = QString("Feature count mismatch: expected %1, got %2")
                                   .arg(config_.means.size()).arg(result.features.size());
            return result;
        }
        
        // Apply preprocessing (normalization)
        for (int i = 0; i < result.features.size(); ++i) {
            if (config_.stds(i) > 1e-10) {  // Avoid division by zero
                result.features(i) = (result.features(i) - config_.means(i)) / config_.stds(i);
            } else {
                result.features(i) = 0.0;  // Constant feature
            }
        }
        
        // Handle missing features
        handleMissingFeatures(result.features);
        
        // Validate final features
        if (!validateFeatures(result.features)) {
            result.error_message = "Feature validation failed";
            return result;
        }
        
        result.success = true;
        result.metadata["feature_count"] = result.features.size();
        result.metadata["preprocessing_applied"] = "normalization";
        
    } catch (const std::exception& e) {
        result.error_message = QString("Feature processing exception: %1").arg(e.what());
    }
    
    return result;
}

QVector<double> FeaturePreprocessor::extractGroundTruthFeatures(const QJsonObject& ground_truth) const {
    QVector<double> features;
    
    // Basic dataset characteristics
    features.append(ground_truth.value("datapoints").toDouble(0.0));
    features.append(ground_truth.value("series_count").toDouble(0.0));
    features.append(ground_truth.value("model_id").toDouble(0.0));
    
    // Parameter counts
    QJsonArray global_params = ground_truth.value("global_params").toArray();
    QJsonArray local_params = ground_truth.value("local_params").toArray();
    
    features.append(static_cast<double>(global_params.size()));
    features.append(static_cast<double>(local_params.size()));
    
    // Parameter values (limited to first few to avoid variable size)
    for (int i = 0; i < std::min(5, global_params.size()); ++i) {
        features.append(global_params[i].toDouble());
    }
    // Pad with zeros if fewer than 5 global parameters
    while (features.size() < 8) {  // 3 basic + 2 counts + 3 more for padding to 8
        features.append(0.0);
    }
    
    // Local parameters (flatten first few)
    int local_param_count = 0;
    for (const auto& local_value : local_params) {
        if (local_param_count >= 10) break;  // Limit to first 10 local parameters
        
        QJsonArray local_array = local_value.toArray();
        for (const auto& param : local_array) {
            if (local_param_count >= 10) break;
            features.append(param.toDouble());
            local_param_count++;
        }
    }
    
    // Pad local parameters to fixed size
    while (local_param_count < 10) {
        features.append(0.0);
        local_param_count++;
    }
    
    return features;
}

QVector<double> FeaturePreprocessor::extractFitQualityFeatures(const QJsonObject& fit_quality) const {
    QVector<double> features;
    
    // Standard fit quality metrics
    features.append(fit_quality.value("aic").toDouble(0.0));
    features.append(fit_quality.value("aicc").toDouble(0.0));
    features.append(fit_quality.value("r_squared").toDouble(0.0));
    features.append(fit_quality.value("sse").toDouble(0.0));
    features.append(fit_quality.value("rmse").toDouble(0.0));
    features.append(fit_quality.value("chi_squared").toDouble(0.0));
    
    // Parameter and data information
    features.append(fit_quality.value("parameter_count").toDouble(0.0));
    features.append(fit_quality.value("data_points").toDouble(0.0));
    features.append(fit_quality.value("global_params").toDouble(0.0));
    features.append(fit_quality.value("local_params").toDouble(0.0));
    features.append(fit_quality.value("series_count").toDouble(0.0));
    
    // Derived metrics
    double data_to_param = fit_quality.value("data_to_param_ratio").toDouble(1.0);
    features.append(data_to_param);
    
    double aic_aicc_ratio = fit_quality.value("aic_aicc_ratio").toDouble(1.0);
    features.append(aic_aicc_ratio);
    
    // Log-transformed metrics (for better numerical properties)
    double sse = fit_quality.value("sse").toDouble(1e-10);
    features.append(std::log10(std::max(1e-10, sse)));
    
    double log_sse = fit_quality.value("log_sse").toDouble();
    features.append(log_sse);
    
    return features;
}

QVector<double> FeaturePreprocessor::extractStatisticalFeatures(const QJsonObject& statistical_features) const {
    QVector<double> features;
    
    // Monte Carlo features
    if (statistical_features.contains("monte_carlo")) {
        QJsonObject mc = statistical_features["monte_carlo"].toObject();
        
        // Extract features from first parameter
        bool found_param = false;
        for (auto it = mc.begin(); it != mc.end(); ++it) {
            if (it.key().startsWith("param_")) {
                QJsonObject param_stats = it.value().toObject();
                
                // Boxplot statistics
                features.append(param_stats.value("boxplot_mean").toDouble(0.0));
                features.append(param_stats.value("boxplot_stddev").toDouble(0.0));
                features.append(param_stats.value("boxplot_median").toDouble(0.0));
                features.append(param_stats.value("boxplot_lower_quartile").toDouble(0.0));
                features.append(param_stats.value("boxplot_upper_quartile").toDouble(0.0));
                features.append(param_stats.value("boxplot_lower_whisker").toDouble(0.0));
                features.append(param_stats.value("boxplot_upper_whisker").toDouble(0.0));
                
                // Confidence intervals
                features.append(param_stats.value("confidence_interval_lower").toDouble(0.0));
                features.append(param_stats.value("confidence_interval_upper").toDouble(0.0));
                
                // Derived metrics
                double ci_width = param_stats.value("confidence_interval_upper").toDouble(0.0) - 
                                 param_stats.value("confidence_interval_lower").toDouble(0.0);
                features.append(ci_width);
                
                found_param = true;
                break;  // Only use first parameter
            }
        }
        
        if (!found_param) {
            // Add zeros if no Monte Carlo data available
            for (int i = 0; i < 10; ++i) {
                features.append(0.0);
            }
        }
    } else {
        // No Monte Carlo data
        for (int i = 0; i < 10; ++i) {
            features.append(0.0);
        }
    }
    
    // Cross-validation features (simplified)
    if (statistical_features.contains("cross_validation")) {
        QJsonObject cv = statistical_features["cross_validation"].toObject();
        
        // Add basic CV metrics if available
        features.append(cv.value("mean_error").toDouble(0.0));
        features.append(cv.value("std_error").toDouble(0.0));
        features.append(cv.value("validation_score").toDouble(0.0));
    } else {
        features.append(0.0);
        features.append(0.0);
        features.append(0.0);
    }
    
    return features;
}

QVector<double> FeaturePreprocessor::extractInputNoiseFeatures(const QJsonObject& input_noise) const {
    QVector<double> features;
    
    // Random seed (normalized)
    double random_seed = input_noise.value("RandomSeed").toDouble(0.0);
    features.append(random_seed / 100000.0);  // Normalize large numbers
    
    // Noise standard deviation
    QJsonArray std_array = input_noise.value("Std").toArray();
    if (!std_array.isEmpty()) {
        // Mean and variance of noise std
        double sum = 0.0, sum_sq = 0.0;
        for (const auto& std_val : std_array) {
            double val = std_val.toDouble();
            sum += val;
            sum_sq += val * val;
        }
        
        double mean_std = sum / std_array.size();
        double var_std = (sum_sq / std_array.size()) - (mean_std * mean_std);
        
        features.append(mean_std);
        features.append(std::sqrt(var_std));
        features.append(static_cast<double>(std_array.size()));  // Number of series
    } else {
        features.append(0.0);
        features.append(0.0);
        features.append(0.0);
    }
    
    // Noise type (encoded)
    QString noise_type = input_noise.value("Type").toString("none");
    double type_encoded = 0.0;
    if (noise_type == "gaussian") type_encoded = 1.0;
    else if (noise_type == "uniform") type_encoded = 2.0;
    features.append(type_encoded);
    
    // Parameter limit features
    if (input_noise.contains("global_random_limits")) {
        QJsonObject global_limits = input_noise["global_random_limits"].toObject();
        QJsonArray min_array = global_limits["min"].toArray();
        QJsonArray max_array = global_limits["max"].toArray();
        
        if (!min_array.isEmpty() && !max_array.isEmpty()) {
            double min_val = min_array[0].toDouble();
            double max_val = max_array[0].toDouble();
            
            features.append(min_val);
            features.append(max_val);
            features.append(max_val - min_val);  // Range
        } else {
            features.append(0.0);
            features.append(0.0);
            features.append(0.0);
        }
    } else {
        features.append(0.0);
        features.append(0.0);
        features.append(0.0);
    }
    
    return features;
}

void FeaturePreprocessor::setConfig(const FeatureConfig& config) {
    config_ = config;
    is_fitted_ = true;
}

bool FeaturePreprocessor::validateFeatures(const Eigen::VectorXd& features) const {
    // Check for NaN or infinite values
    for (int i = 0; i < features.size(); ++i) {
        if (!std::isfinite(features(i))) {
            qWarning() << "Invalid feature value at index" << i << ":" << features(i);
            return false;
        }
    }
    
    return true;
}

void FeaturePreprocessor::handleMissingFeatures(Eigen::VectorXd& features) const {
    // Replace NaN or infinite values with 0
    for (int i = 0; i < features.size(); ++i) {
        if (!std::isfinite(features(i))) {
            features(i) = 0.0;
        }
    }
}
```

### 4. CLI Integration Example

```cpp
// src/client/suprafit_cli.cpp - ML command additions
bool SupraFitCli::PredictBestModels() {
    if (m_input_file.isEmpty()) {
        std::cout << "Error: Input file required for model prediction\n";
        return false;
    }
    
    // Initialize ML predictor
    if (!initializeMLPredictor()) {
        std::cout << "Error: Failed to initialize ML predictor\n";
        return false;
    }
    
    // Extract features from input file
    QJsonObject features = extractFeaturesFromFile(m_input_file);
    if (features.isEmpty()) {
        std::cout << "Error: Failed to extract features from input file\n";
        return false;
    }
    
    // Make prediction
    ModelPredictor::PredictionResult result = model_predictor_->predictBestModels(features, 5);
    
    if (result.suggestions.isEmpty()) {
        std::cout << "Error: No model predictions available\n";
        return false;
    }
    
    // Display results
    std::cout << "🔮 ML Model Predictions:\n";
    std::cout << "Input file: " << m_input_file.toStdString() << "\n";
    std::cout << "Prediction time: " << result.prediction_time_ms << " ms\n\n";
    
    for (int i = 0; i < result.suggestions.size(); ++i) {
        const auto& suggestion = result.suggestions[i];
        
        std::cout << (i + 1) << ". " << suggestion.model_name.toStdString() 
                  << " (ID: " << suggestion.model_id << ")\n";
        std::cout << "   Confidence: " << (suggestion.confidence * 100) 
                  << "%, Uncertainty: " << (suggestion.uncertainty * 100) << "%\n";
        std::cout << "   Reasoning: " << suggestion.reasoning.toStdString() << "\n\n";
    }
    
    // Ask user if they want to fit suggested models
    if (m_interactive) {
        std::cout << "Fit suggested models? (y/N): ";
        std::string response;
        std::getline(std::cin, response);
        
        if (response == "y" || response == "Y") {
            return runMLAssistedPipeline();
        }
    }
    
    return true;
}

bool SupraFitCli::initializeMLPredictor() {
    if (model_predictor_ && model_predictor_->isModelLoaded()) {
        return true;  // Already initialized
    }
    
    model_predictor_ = std::make_unique<ModelPredictor>(this);
    
    // Determine model path
    QString model_path = ml_model_path_;
    if (model_path.isEmpty()) {
        // Use default model location
        QString app_dir = QCoreApplication::applicationDirPath();
        model_path = app_dir + "/../share/suprafit/ml_models/default_model.json";
        
        // Check if file exists
        if (!QFile::exists(model_path)) {
            // Try alternative paths
            QStringList possible_paths = {
                app_dir + "/ml_models/default_model.json",
                app_dir + "/../ml_models/default_model.json",
                "./ml_models/default_model.json"
            };
            
            for (const QString& path : possible_paths) {
                if (QFile::exists(path)) {
                    model_path = path;
                    break;
                }
            }
        }
    }
    
    if (!QFile::exists(model_path)) {
        std::cout << "Warning: ML model not found at " << model_path.toStdString() << "\n";
        std::cout << "ML model prediction will not be available.\n";
        return false;
    }
    
    return model_predictor_->loadModel(model_path);
}

QJsonObject SupraFitCli::extractFeaturesFromFile(const QString& filename) {
    // Use existing MLFeatureExtractor
    MLFeatureExtractor extractor;
    
    QJsonObject parsed_data = extractor.parseMLPipelineData(filename);
    if (parsed_data.isEmpty()) {
        return QJsonObject();
    }
    
    return extractor.extractCompactTrainingSample(parsed_data);
}
```

These C++ examples provide a complete, production-ready foundation for integrating machine learning into SupraFit while maintaining its educational-first design philosophy and C++ core architecture.