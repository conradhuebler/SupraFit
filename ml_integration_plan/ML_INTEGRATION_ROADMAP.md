# SupraFit Machine Learning Integration Plan

## Project Vision
Integrate neural network-based model selection into SupraFit as a native C++ feature, eliminating manual model selection and improving accuracy for complex supramolecular systems.

## Architecture Overview

### Phase 1: C++ Neural Network Infrastructure (Pure C++)
```
SupraFit Core
├── src/ml/
│   ├── neural_network.cpp      # Core NN implementation
│   ├── model_predictor.cpp     # Model selection predictor
│   ├── feature_preprocessor.cpp # Feature normalization/scaling
│   ├── onnx_loader.cpp         # ONNX model loading (inference only)
│   └── training_export.cpp     # Enhanced MLFeatureExtractor integration
├── external/
│   ├── onnxruntime/           # C++ inference engine
│   └── eigen/                 # Already available for matrix ops
```

### Phase 2: Python Training Pipeline (Optional)
```
training/
├── train_model.py            # Neural network training
├── data_preprocessing.py     # Feature engineering
├── model_export.py          # Export to ONNX format
├── hyperparameter_tuning.py # Optimization
└── validation.py            # Cross-validation & testing
```

## Technical Implementation Strategy

### 1. C++ Neural Network Implementation (Primary Approach)

#### Option A: Lightweight Custom Implementation
**Benefits**: No external dependencies, full control, educational value
**Components**:
- Matrix operations using existing Eigen library
- Feed-forward networks with backpropagation
- Basic activation functions (ReLU, sigmoid, softmax)
- Adam optimizer implementation

```cpp
// src/ml/neural_network.h
class NeuralNetwork {
public:
    NeuralNetwork(const std::vector<int>& layer_sizes);
    
    // Training (for future expansion)
    void train(const Eigen::MatrixXd& X, const Eigen::MatrixXd& y, 
               int epochs = 1000, double learning_rate = 0.001);
    
    // Inference (primary use case)
    Eigen::VectorXd predict(const Eigen::VectorXd& input);
    
    // Model persistence
    bool saveModel(const QString& filename);
    bool loadModel(const QString& filename);
    
private:
    std::vector<Eigen::MatrixXd> weights_;
    std::vector<Eigen::VectorXd> biases_;
    std::vector<int> layer_sizes_;
};
```

#### Option B: ONNX Runtime Integration (Recommended)
**Benefits**: Industry standard, excellent performance, pre-trained model compatibility
**Implementation**: C++ ONNX Runtime for inference, Python for training

```cpp
// src/ml/onnx_predictor.h
class ONNXPredictor {
public:
    bool loadModel(const QString& model_path);
    QVector<double> predict(const QVector<double>& features);
    QString getPredictedModel() const;
    
private:
    std::unique_ptr<Ort::Session> session_;
    std::vector<const char*> input_names_;
    std::vector<const char*> output_names_;
};
```

### 2. Model Selection Integration

#### Enhanced Model Predictor
```cpp
// src/ml/model_predictor.h
class ModelPredictor : public QObject {
    Q_OBJECT
    
public:
    struct ModelPrediction {
        QString model_name;
        int model_id;
        double confidence;
        QJsonObject reasoning;  // Feature importance, uncertainty
    };
    
    // Predict best model from extracted features
    ModelPrediction predictBestModel(const QJsonObject& features);
    
    // Predict ranking of multiple models
    QVector<ModelPrediction> rankModels(const QJsonObject& features);
    
    // Integration with existing SupraFit workflow
    QVector<int> suggestModelsToFit(const QJsonObject& features, 
                                    int max_models = 3);

private:
    std::unique_ptr<ONNXPredictor> predictor_;
    FeaturePreprocessor preprocessor_;
};
```

### 3. Feature Preprocessing Pipeline

```cpp
// src/ml/feature_preprocessor.h
class FeaturePreprocessor {
public:
    struct ProcessedFeatures {
        Eigen::VectorXd features;
        QStringList feature_names;
        QJsonObject metadata;
    };
    
    // Convert MLFeatureExtractor output to neural network input
    ProcessedFeatures processFeatures(const QJsonObject& raw_features);
    
    // Feature scaling and normalization
    void fitScaler(const QVector<QJsonObject>& training_data);
    Eigen::VectorXd transform(const QJsonObject& features);
    
    // Feature selection and importance
    QVector<int> selectImportantFeatures(int n_features = 20);
    
private:
    // Scaling parameters
    Eigen::VectorXd feature_means_;
    Eigen::VectorXd feature_stds_;
    QVector<int> selected_features_;
};
```

## Integration with Existing SupraFit Architecture

### 1. CLI Enhancement

```cpp
// src/client/suprafit_cli.h - additions
class SupraFitCli {
public:
    // ML model prediction
    bool PredictBestModels();
    bool AutoModelSelection();
    
    // Training data workflows  
    bool ExportMLTrainingBatch();
    bool ValidateMLModel();
    
private:
    std::unique_ptr<ModelPredictor> model_predictor_;
};
```

### 2. GUI Integration

```cpp
// src/ui/ml_assistant_widget.h
class MLAssistantWidget : public QWidget {
    Q_OBJECT
    
public:
    void setData(QSharedPointer<DataClass> data);
    
public slots:
    void predictModels();
    void showModelReasoning();
    void acceptSuggestion();
    
private:
    ModelPredictor* predictor_;
    QListWidget* model_suggestions_;
    QTextEdit* reasoning_display_;
};
```

### 3. Enhanced ML Pipeline

```cpp
// Existing MLFeatureExtractor + New ModelPredictor integration
class EnhancedMLPipeline {
public:
    struct MLResult {
        QJsonObject features;           // From MLFeatureExtractor
        QVector<ModelPrediction> predictions;  // From ModelPredictor
        QJsonObject fit_results;        // From SupraFit fitting
        QJsonObject comparison;         // Performance comparison
    };
    
    MLResult processDataset(const QString& filename);
    bool trainFromBatch(const QStringList& training_files);
};
```

## Neural Network Architecture Design

### Input Features (from MLFeatureExtractor)
```cpp
struct InputFeatures {
    // Ground truth information (20 features)
    double datapoints, series_count;
    QVector<double> global_params, local_params;
    
    // Input noise parameters (8 features)  
    double noise_std_mean, noise_std_var;
    QString noise_type;  // encoded as categorical
    QVector<double> param_limits;
    
    // Fit quality metrics (15 features)
    double aic, aicc, r_squared, sse, rmse, chi_squared;
    double param_count, data_to_param_ratio;
    
    // Statistical analysis features (30+ features)
    QJsonObject monte_carlo_stats;
    QJsonObject cross_validation_stats;
    QJsonObject reduction_analysis;
    
    // Derived features (10 features)
    double complexity_score, noise_level, convergence_quality;
};
```

### Network Architecture
```
Input Layer:    ~80 features
Hidden Layer 1: 128 neurons (ReLU)
Hidden Layer 2: 64 neurons (ReLU) 
Hidden Layer 3: 32 neurons (ReLU)
Output Layer:   N classes (softmax) where N = number of model types
```

### Output Classes (Model Types)
- NMR 1:1 binding (ID: 1)
- NMR 1:2 binding (ID: 2)  
- NMR 2:1 binding (ID: 3)
- ITC models (ID: 10-15)
- UV-Vis models (ID: 20-25)
- Custom script models (ID: 100+)

## Implementation Phases

### Phase 1: Foundation (4-6 weeks)
1. **Neural Network Core** (2 weeks)
   - Basic feed-forward implementation in C++
   - Matrix operations using Eigen
   - JSON model serialization

2. **ONNX Integration** (1 week)
   - Add ONNX Runtime to CMakeLists.txt
   - Implement ONNXPredictor class
   - Test with dummy models

3. **Feature Pipeline** (2 weeks)
   - Enhance MLFeatureExtractor with standardized output
   - Implement FeaturePreprocessor 
   - Feature scaling and selection

4. **CLI Integration** (1 week)
   - Add `--predict-models` command
   - Integration with existing ML pipeline
   - Testing with real data

### Phase 2: Training Pipeline (2-3 weeks)
1. **Python Training Scripts** (1 week)
   - Data loading from MLFeatureExtractor output
   - Neural network training with PyTorch/TensorFlow
   - Hyperparameter optimization

2. **Model Export** (1 week)
   - ONNX export from Python models
   - Validation of C++ inference
   - Model versioning and metadata

3. **Validation Framework** (1 week)
   - Cross-validation on SupraFit datasets
   - Performance benchmarking
   - Accuracy metrics and reporting

### Phase 3: Advanced Features (3-4 weeks)
1. **GUI Integration** (2 weeks)
   - ML Assistant widget
   - Visual model suggestions
   - Interactive reasoning display

2. **Uncertainty Quantification** (1 week)
   - Bayesian neural networks
   - Confidence intervals for predictions
   - Model uncertainty visualization

3. **Active Learning** (1 week)
   - Identify datasets needing more training data
   - Adaptive model improvement
   - Online learning capabilities

## Training Data Strategy

### Data Generation Pipeline
```bash
# Generate diverse training datasets
./suprafit_cli --generate-ml-training-batch \
    --models "1,2,3,10,11,12" \
    --samples-per-model 1000 \
    --noise-levels "0.001,0.005,0.01" \
    --output-dir training_data/

# Extract features for training
./suprafit_cli --export-ml-training-batch \
    --input-dir training_data/ \
    --ml-output consolidated_training.json

# Train neural network (Python)
python training/train_model.py \
    --input consolidated_training.json \
    --output model_v1.onnx \
    --epochs 1000
```

### Training Dataset Requirements
- **Size**: 10,000+ samples per model type
- **Diversity**: Various noise levels, parameter ranges, data sizes  
- **Balance**: Equal representation of all model types
- **Validation**: 20% holdout for testing
- **Augmentation**: Synthetic data generation with different conditions

## C++ Dependencies and Build System

### CMakeLists.txt Additions
```cmake
# ML dependencies
option(BUILD_ML_SUPPORT "Build with Machine Learning support" ON)

if(BUILD_ML_SUPPORT)
    # ONNX Runtime
    find_package(onnxruntime REQUIRED)
    target_link_libraries(core onnxruntime)
    
    # Add ML source files
    target_sources(core PRIVATE
        src/ml/neural_network.cpp
        src/ml/onnx_predictor.cpp
        src/ml/model_predictor.cpp
        src/ml/feature_preprocessor.cpp
    )
    
    target_compile_definitions(core PRIVATE ML_SUPPORT_ENABLED)
endif()
```

### External Dependencies
1. **ONNX Runtime** (C++): Model inference
2. **Eigen** (Already available): Matrix operations
3. **Qt6** (Already available): JSON handling, file I/O

### Optional Python Dependencies (Training only)
```python
# requirements.txt
torch>=1.9.0
onnx>=1.10.0  
scikit-learn>=1.0.0
pandas>=1.3.0
numpy>=1.21.0
matplotlib>=3.4.0
```

## Testing and Validation Strategy

### Unit Tests
```cpp
// src/tests/test_ml_predictor.cpp
class TestMLPredictor : public QObject {
    Q_OBJECT
    
private slots:
    void testFeatureExtraction();
    void testPreprocessing();
    void testModelPrediction();
    void testONNXLoading();
};
```

### Integration Tests  
```bash
# End-to-end ML pipeline test
./suprafit_cli -i test_data.json --predict-models --validate-prediction
```

### Benchmarking
- **Accuracy**: >90% correct model prediction on test set
- **Speed**: <100ms prediction time per dataset
- **Memory**: <500MB model size for production use

## Deployment and Distribution

### Model Distribution
1. **Pre-trained Models**: Ship with SupraFit installation
2. **Model Updates**: Download mechanism for improved models
3. **Custom Models**: User-trainable models for specific domains

### Documentation
- User guide for ML-assisted model selection
- Developer documentation for extending ML capabilities
- Training pipeline documentation for creating custom models

## Future Enhancements

### Advanced ML Features
1. **Multi-task Learning**: Simultaneous parameter estimation
2. **Transfer Learning**: Adapt to new experimental conditions
3. **Reinforcement Learning**: Optimize experimental design
4. **Graph Neural Networks**: Molecular structure awareness

### Integration Possibilities
1. **Automated Experimental Design**: Suggest optimal data points
2. **Real-time Analysis**: Streaming data processing  
3. **Collaborative Learning**: Learn from community data
4. **Uncertainty-guided Sampling**: Active learning for efficient experiments

## Risk Assessment and Mitigation

### Technical Risks
1. **Model Accuracy**: Extensive validation with diverse datasets
2. **Performance**: Optimize C++ implementation and model size
3. **Compatibility**: Thorough testing across platforms
4. **Maintenance**: Clear separation of training and inference code

### User Adoption Risks  
1. **Complexity**: Provide simple one-click model selection
2. **Trust**: Transparent reasoning and confidence intervals
3. **Fallback**: Always allow manual model selection override

## Success Metrics

### Technical Metrics
- **Prediction Accuracy**: >90% on validation set
- **Speed**: <100ms per prediction
- **Memory Usage**: <100MB additional RAM
- **Build Size**: <50MB increase in binary size

### User Experience Metrics
- **Adoption Rate**: % of users using ML suggestions
- **Accuracy Improvement**: % improvement in model selection
- **Time Savings**: Reduction in manual model exploration time
- **User Satisfaction**: Feedback scores for ML assistance

This comprehensive plan provides a roadmap for integrating machine learning into SupraFit while maintaining its C++ core philosophy and ensuring practical deployment without Python runtime dependencies.