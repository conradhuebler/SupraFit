# SupraFit ML Technical Architecture

⚠️ **IMPORTANT NOTICE: This is an ARCHITECTURAL PLANNING DOCUMENT**

The code examples and component specifications shown below represent the **planned architecture** for future implementation. The classes, files, and implementations described here are **NOT YET IMPLEMENTED** in the codebase.

**Status**:
- ✅ Architecture has been designed and validated
- 🚧 Implementation is pending
- 📖 This document serves as a specification guide for future development

**Do Not**:
- Try to `#include` the header files mentioned below (they don't exist yet)
- Look for the implementations in `src/ml/` directory (they are planned for future development)
- Use these APIs in your code (they are not yet available)

**Refer to**:
- `ml_integration_plan/README.md` for current implementation status
- `src/client/suprafit_cli.cpp` for currently available ML pipeline features
- `examples/ml_pipeline/` for working examples of the available workflow

---

## Core C++ Neural Network Implementation (Planned)

### 1. Lightweight Neural Network Engine

```cpp
// src/ml/neural_network.h
#pragma once

#include <vector>
#include <memory>
#include <Eigen/Dense>
#include <QtCore/QJsonObject>
#include <QtCore/QString>

class ActivationFunction {
public:
    virtual ~ActivationFunction() = default;
    virtual double forward(double x) const = 0;
    virtual double backward(double x) const = 0;  // derivative
    virtual QString name() const = 0;
};

class ReLUActivation : public ActivationFunction {
public:
    double forward(double x) const override { return std::max(0.0, x); }
    double backward(double x) const override { return x > 0.0 ? 1.0 : 0.0; }
    QString name() const override { return "ReLU"; }
};

class SigmoidActivation : public ActivationFunction {
public:
    double forward(double x) const override { 
        return 1.0 / (1.0 + std::exp(-x)); 
    }
    double backward(double x) const override {
        double s = forward(x);
        return s * (1.0 - s);
    }
    QString name() const override { return "Sigmoid"; }
};

class SoftmaxActivation : public ActivationFunction {
public:
    // Softmax is applied to entire layer, not individual neurons
    Eigen::VectorXd forwardVector(const Eigen::VectorXd& x) const;
    double forward(double x) const override { return x; } // Not used for softmax
    double backward(double x) const override { return 1.0; } // Not used for softmax
    QString name() const override { return "Softmax"; }
};

class NeuralLayer {
public:
    NeuralLayer(int input_size, int output_size, 
                std::unique_ptr<ActivationFunction> activation);
    
    // Forward propagation
    Eigen::VectorXd forward(const Eigen::VectorXd& input);
    
    // Backward propagation (for training)
    Eigen::VectorXd backward(const Eigen::VectorXd& grad_output);
    
    // Parameter access
    const Eigen::MatrixXd& weights() const { return weights_; }
    const Eigen::VectorXd& biases() const { return biases_; }
    
    // Parameter updates
    void updateWeights(const Eigen::MatrixXd& weight_gradients, double learning_rate);
    void updateBiases(const Eigen::VectorXd& bias_gradients, double learning_rate);
    
    // Serialization
    QJsonObject toJson() const;
    bool fromJson(const QJsonObject& json);
    
private:
    Eigen::MatrixXd weights_;
    Eigen::VectorXd biases_;
    std::unique_ptr<ActivationFunction> activation_;
    
    // Cache for backward pass
    Eigen::VectorXd last_input_;
    Eigen::VectorXd last_output_;
    int input_size_, output_size_;
    
    void initializeWeights();  // Xavier/He initialization
};

class NeuralNetwork {
public:
    NeuralNetwork();
    explicit NeuralNetwork(const std::vector<int>& layer_sizes);
    
    // Network construction
    void addLayer(int input_size, int output_size, const QString& activation = "ReLU");
    void finalize();  // Prepare for training/inference
    
    // Inference
    Eigen::VectorXd predict(const Eigen::VectorXd& input);
    QVector<double> predictVector(const QVector<double>& input);
    
    // Training
    struct TrainingConfig {
        int epochs = 1000;
        double learning_rate = 0.001;
        int batch_size = 32;
        double validation_split = 0.2;
        bool early_stopping = true;
        int patience = 50;
        QString optimizer = "Adam";  // "SGD", "Adam"
    };
    
    struct TrainingHistory {
        QVector<double> train_loss;
        QVector<double> val_loss;
        QVector<double> train_accuracy;
        QVector<double> val_accuracy;
        int best_epoch;
        double best_val_loss;
    };
    
    TrainingHistory train(const Eigen::MatrixXd& X, const Eigen::MatrixXd& y, 
                         const TrainingConfig& config = TrainingConfig());
    
    // Model persistence
    bool saveModel(const QString& filename) const;
    bool loadModel(const QString& filename);
    
    // Model information
    QStringList getModelInfo() const;
    int getParameterCount() const;
    
private:
    std::vector<std::unique_ptr<NeuralLayer>> layers_;
    bool is_finalized_;
    
    // Training utilities
    double computeLoss(const Eigen::MatrixXd& predictions, const Eigen::MatrixXd& targets) const;
    double computeAccuracy(const Eigen::MatrixXd& predictions, const Eigen::MatrixXd& targets) const;
    void shuffleData(Eigen::MatrixXd& X, Eigen::MatrixXd& y) const;
};
```

### 2. Model Predictor Integration

```cpp
// src/ml/model_predictor.h
#pragma once

#include <QtCore/QObject>
#include <QtCore/QJsonObject>
#include <QtCore/QVector>
#include "neural_network.h"

class ModelPredictor : public QObject {
    Q_OBJECT
    
public:
    struct ModelSuggestion {
        int model_id;
        QString model_name;
        double confidence;
        double uncertainty;
        QJsonObject feature_importance;
        QString reasoning;
    };
    
    struct PredictionResult {
        QVector<ModelSuggestion> suggestions;
        QJsonObject input_features;
        QJsonObject metadata;
        double prediction_time_ms;
    };
    
    explicit ModelPredictor(QObject* parent = nullptr);
    ~ModelPredictor();
    
    // Model management
    bool loadModel(const QString& model_path);
    bool isModelLoaded() const;
    QString getModelInfo() const;
    
    // Prediction
    PredictionResult predictBestModels(const QJsonObject& features, int top_k = 3);
    ModelSuggestion predictSingleBest(const QJsonObject& features);
    
    // Integration with SupraFit workflow
    QVector<int> suggestModelIds(const QJsonObject& features, int max_models = 5);
    bool shouldFitModel(int model_id, const QJsonObject& features, double threshold = 0.1);
    
    // Batch prediction
    QVector<PredictionResult> predictBatch(const QVector<QJsonObject>& features_batch);
    
    // Model validation and diagnostics
    bool validateModel(const QString& test_data_path);
    QJsonObject getModelDiagnostics() const;
    
signals:
    void predictionCompleted(const PredictionResult& result);
    void modelLoaded(const QString& model_path);
    void errorOccurred(const QString& error);
    
private slots:
    void handlePredictionError(const QString& error);
    
private:
    std::unique_ptr<NeuralNetwork> network_;
    std::unique_ptr<class FeaturePreprocessor> preprocessor_;
    
    // Model metadata
    QJsonObject model_metadata_;
    QStringList model_names_;
    QVector<int> model_ids_;
    
    // Performance tracking
    mutable double total_prediction_time_;
    mutable int prediction_count_;
    
    // Internal methods
    QJsonObject extractFeatureImportance(const Eigen::VectorXd& features) const;
    QString generateReasoning(const ModelSuggestion& suggestion, 
                             const QJsonObject& input_features) const;
    double estimateUncertainty(const Eigen::VectorXd& prediction) const;
};
```

### 3. Feature Preprocessing Pipeline

```cpp
// src/ml/feature_preprocessor.h
#pragma once

#include <Eigen/Dense>
#include <QtCore/QJsonObject>
#include <QtCore/QStringList>
#include <QtCore/QVector>

class FeaturePreprocessor {
public:
    struct FeatureConfig {
        QStringList feature_names;
        Eigen::VectorXd means;
        Eigen::VectorXd stds;
        Eigen::VectorXd mins;
        Eigen::VectorXd maxs;
        QVector<int> categorical_indices;
        QJsonObject encoding_maps;
    };
    
    struct ProcessedFeatures {
        Eigen::VectorXd features;
        QStringList feature_names;
        QJsonObject metadata;
        bool success;
        QString error_message;
    };
    
    FeaturePreprocessor();
    
    // Configuration
    bool loadConfig(const QString& config_path);
    bool saveConfig(const QString& config_path) const;
    void setConfig(const FeatureConfig& config);
    
    // Feature processing
    ProcessedFeatures processFeatures(const QJsonObject& raw_features) const;
    
    // Training data processing (for fitting scaler)
    bool fitTransform(const QVector<QJsonObject>& training_data);
    
    // Feature extraction from MLFeatureExtractor output
    static QJsonObject extractNumericalFeatures(const QJsonObject& ml_output);
    static QJsonObject extractCategoricalFeatures(const QJsonObject& ml_output);
    static QJsonObject extractStatisticalFeatures(const QJsonObject& ml_output);
    
private:
    FeatureConfig config_;
    bool is_fitted_;
    
    // Feature extraction methods
    QVector<double> extractGroundTruthFeatures(const QJsonObject& ground_truth) const;
    QVector<double> extractFitQualityFeatures(const QJsonObject& fit_quality) const;
    QVector<double> extractStatisticalFeatures(const QJsonObject& statistical_features) const;
    QVector<double> extractInputNoiseFeatures(const QJsonObject& input_noise) const;
    
    // Preprocessing utilities
    double normalizeFeature(double value, double mean, double std) const;
    double minMaxScale(double value, double min, double max) const;
    int encodeCategory(const QString& category, const QString& feature_name) const;
    
    // Statistical computations
    void computeStatistics(const QVector<QVector<double>>& data);
    
    // Validation
    bool validateFeatures(const Eigen::VectorXd& features) const;
    void handleMissingFeatures(Eigen::VectorXd& features) const;
};
```

## Integration with Existing SupraFit Architecture

### 1. Enhanced CLI Commands

```cpp
// src/client/suprafit_cli.h - ML additions
public:
    // ML model prediction commands
    bool PredictBestModels();
    bool AutoModelSelection();
    bool ValidateMLModel();
    
    // Training data management
    bool GenerateMLTrainingBatch();
    bool ConsolidateTrainingData();
    
    // Model management
    bool ListAvailableMLModels();
    bool UpdateMLModel();

private:
    // ML components
    std::unique_ptr<ModelPredictor> model_predictor_;
    QString ml_model_path_;
    
    // ML workflow methods
    bool initializeMLPredictor();
    QJsonObject extractFeaturesFromFile(const QString& filename);
    bool runMLAssistedPipeline();
```

### 2. ML Pipeline Integration

```cpp
// src/client/ml_pipeline.h
#pragma once

#include <QtCore/QObject>
#include "model_predictor.h"
#include "../capabilities/mlfeatureextractor.h"

class MLPipeline : public QObject {
    Q_OBJECT
    
public:
    struct PipelineConfig {
        QString input_file;
        QString output_dir;
        int max_models_to_fit = 3;
        double confidence_threshold = 0.3;
        bool run_statistical_analysis = true;
        bool export_training_data = false;
        QString ml_model_path;
    };
    
    struct PipelineResult {
        QJsonObject original_features;
        QVector<ModelPredictor::ModelSuggestion> suggestions;
        QJsonObject fit_results;
        QJsonObject performance_comparison;
        double total_time_ms;
        bool success;
        QString error_message;
    };
    
    explicit MLPipeline(QObject* parent = nullptr);
    
    // Pipeline execution
    PipelineResult runFullPipeline(const PipelineConfig& config);
    PipelineResult runPredictionOnly(const QString& input_file, 
                                   const QString& ml_model_path);
    
    // Batch processing
    QVector<PipelineResult> runBatchPipeline(const QStringList& input_files,
                                           const PipelineConfig& config);
    
    // Components access
    ModelPredictor* getPredictor() const { return predictor_.get(); }
    MLFeatureExtractor* getFeatureExtractor() const { return extractor_.get(); }
    
signals:
    void pipelineProgress(int percentage, const QString& status);
    void modelFittingStarted(int model_id, const QString& model_name);
    void modelFittingCompleted(int model_id, const QJsonObject& results);
    void pipelineCompleted(const PipelineResult& result);
    
private:
    std::unique_ptr<ModelPredictor> predictor_;
    std::unique_ptr<MLFeatureExtractor> extractor_;
    
    // Pipeline steps
    QJsonObject extractFeatures(const QString& input_file);
    QVector<int> selectModelsToFit(const QJsonObject& features, int max_models);
    QJsonObject fitSelectedModels(const QVector<int>& model_ids, 
                                 const QString& input_file);
    QJsonObject compareResults(const QVector<int>& model_ids, 
                              const QVector<ModelPredictor::ModelSuggestion>& suggestions,
                              const QJsonObject& fit_results);
};
```

### 3. CMake Integration

```cmake
# CMakeLists.txt additions for ML support
option(BUILD_ML_SUPPORT "Build with Machine Learning support" ON)
option(USE_ONNX_RUNTIME "Use ONNX Runtime for neural network inference" OFF)

if(BUILD_ML_SUPPORT)
    message(STATUS "Building with Machine Learning support")
    
    # ML source files
    set(ml_SRC
        src/ml/neural_network.cpp
        src/ml/model_predictor.cpp
        src/ml/feature_preprocessor.cpp
        src/ml/ml_pipeline.cpp
    )
    
    # Add ML sources to core library
    target_sources(core PRIVATE ${ml_SRC})
    
    # Compile definitions
    target_compile_definitions(core PRIVATE ML_SUPPORT_ENABLED)
    
    # ONNX Runtime integration (optional)
    if(USE_ONNX_RUNTIME)
        find_package(onnxruntime QUIET)
        if(onnxruntime_FOUND)
            target_sources(core PRIVATE src/ml/onnx_predictor.cpp)
            target_link_libraries(core onnxruntime)
            target_compile_definitions(core PRIVATE ONNX_SUPPORT_ENABLED)
            message(STATUS "ONNX Runtime support enabled")
        else()
            message(WARNING "ONNX Runtime not found, using built-in neural network")
        endif()
    endif()
    
    # Install ML models
    install(DIRECTORY models/ml/ 
            DESTINATION share/suprafit/ml_models
            FILES_MATCHING PATTERN "*.json" PATTERN "*.onnx")
            
else()
    message(STATUS "Machine Learning support disabled")
endif()
```

## Model File Formats and Storage

### 1. Native SupraFit Neural Network Format

```json
{
    "model_metadata": {
        "version": "1.0",
        "created": "2025-01-15T10:30:00Z",
        "architecture": "feedforward",
        "input_size": 83,
        "output_size": 6,
        "training_samples": 50000,
        "validation_accuracy": 0.94,
        "model_type": "classification"
    },
    "feature_config": {
        "feature_names": ["datapoints", "series_count", "aic", "r_squared", ...],
        "means": [12.5, 2.3, -45.2, 0.987, ...],
        "stds": [5.2, 0.8, 15.3, 0.025, ...],
        "categorical_features": ["noise_type", "model_family"],
        "encoding_maps": {
            "noise_type": {"gaussian": 0, "uniform": 1, "none": 2}
        }
    },
    "network_architecture": {
        "layers": [
            {
                "type": "dense",
                "input_size": 83,
                "output_size": 128,
                "activation": "ReLU",
                "weights": [[0.123, -0.456, ...], ...],
                "biases": [0.1, -0.05, ...]
            },
            {
                "type": "dense", 
                "input_size": 128,
                "output_size": 64,
                "activation": "ReLU",
                "weights": [[...], ...],
                "biases": [...]
            },
            {
                "type": "dense",
                "input_size": 64,
                "output_size": 6,
                "activation": "Softmax",
                "weights": [[...], ...],
                "biases": [...]
            }
        ]
    },
    "class_labels": {
        "0": {"id": 1, "name": "NMR 1:1", "family": "nmr"},
        "1": {"id": 2, "name": "NMR 1:2", "family": "nmr"}, 
        "2": {"id": 3, "name": "NMR 2:1", "family": "nmr"},
        "3": {"id": 10, "name": "ITC 1:1", "family": "itc"},
        "4": {"id": 11, "name": "ITC 1:2", "family": "itc"},
        "5": {"id": 20, "name": "UV-Vis 1:1", "family": "uvvis"}
    }
}
```

### 2. Model Directory Structure

```
share/suprafit/ml_models/
├── default/
│   ├── model_v1.0.json          # Default trained model
│   ├── feature_config.json      # Feature preprocessing config
│   └── model_metadata.json      # Version info, changelog
├── specialized/
│   ├── nmr_specialist_v1.json   # NMR-specific model
│   ├── itc_specialist_v1.json   # ITC-specific model
│   └── uvvis_specialist_v1.json # UV-Vis specific model
├── experimental/
│   └── beta_model_v2.json       # Development/testing models
└── user_models/
    └── custom_model.json        # User-trained models
```

## Performance Optimization Strategies

### 1. Memory Management

```cpp
// Efficient memory usage for large models
class ModelCache {
public:
    static ModelCache& instance();
    
    std::shared_ptr<NeuralNetwork> getModel(const QString& model_path);
    void preloadModels(const QStringList& model_paths);
    void clearCache();
    void setMaxMemoryMB(int max_mb);
    
private:
    QHash<QString, std::weak_ptr<NeuralNetwork>> cache_;
    int max_memory_mb_;
    
    void evictLRU();
    int getCurrentMemoryUsage() const;
};
```

### 2. Batch Processing Optimization

```cpp
// Optimized batch prediction
class BatchPredictor {
public:
    struct BatchResult {
        QVector<ModelPredictor::PredictionResult> results;
        double total_time_ms;
        int processed_count;
        int failed_count;
    };
    
    BatchResult predictBatch(const QVector<QJsonObject>& features_batch,
                           int batch_size = 32);
    
private:
    // Process multiple samples simultaneously
    Eigen::MatrixXd prepareBatchFeatures(const QVector<QJsonObject>& batch);
    QVector<ModelPredictor::PredictionResult> processBatchResults(
        const Eigen::MatrixXd& predictions, const QVector<QJsonObject>& inputs);
};
```

### 3. Parallel Processing

```cpp
// Thread-safe parallel prediction
class ParallelMLPredictor {
public:
    ParallelMLPredictor(int num_threads = QThread::idealThreadCount());
    
    QFuture<ModelPredictor::PredictionResult> predictAsync(const QJsonObject& features);
    QVector<QFuture<ModelPredictor::PredictionResult>> predictBatchAsync(
        const QVector<QJsonObject>& features_batch);
    
private:
    QThreadPool thread_pool_;
    QVector<std::unique_ptr<ModelPredictor>> predictors_;  // One per thread
};
```

## Error Handling and Validation

### 1. Robust Error Handling

```cpp
// Comprehensive error handling
class MLError {
public:
    enum Type {
        ModelNotFound,
        FeatureExtractionFailed,
        InvalidFeatures,
        PredictionFailed,
        ModelLoadError,
        ConfigurationError
    };
    
    MLError(Type type, const QString& message, const QString& context = QString());
    
    Type type() const { return type_; }
    QString message() const { return message_; }
    QString context() const { return context_; }
    QString toString() const;
    
private:
    Type type_;
    QString message_;
    QString context_;
};

class MLValidator {
public:
    static bool validateFeatures(const QJsonObject& features, QString* error = nullptr);
    static bool validateModel(const QString& model_path, QString* error = nullptr);
    static bool validatePrediction(const ModelPredictor::PredictionResult& result, 
                                  QString* error = nullptr);
    
private:
    static QStringList required_features_;
    static QHash<QString, QPair<double, double>> feature_ranges_;
};
```

### 2. Graceful Fallback Mechanisms

```cpp
// Fallback to traditional model selection if ML fails
class HybridModelSelector {
public:
    QVector<int> selectModels(const QJsonObject& features);
    
private:
    std::unique_ptr<ModelPredictor> ml_predictor_;
    
    // Fallback heuristics
    QVector<int> heuristicModelSelection(const QJsonObject& features);
    QVector<int> statisticalModelSelection(const QJsonObject& features);
    int estimateModelComplexity(const QJsonObject& features);
};
```

This technical architecture provides a comprehensive foundation for integrating machine learning into SupraFit while maintaining its C++ core philosophy and ensuring robust, production-ready functionality.