# SupraFit Machine Learning Integration Plan

## Overview

This directory contains comprehensive planning documentation for integrating machine learning capabilities into SupraFit. The goal is to provide neural network-based model selection that eliminates manual model exploration and improves accuracy for complex supramolecular systems.

## Key Design Principles

- **Pure C++ Runtime**: No Python dependencies for end users
- **Educational-First**: Transparent algorithms with clear scientific meaning
- **Optional Training**: Python used only for model training, not inference
- **SupraFit Integration**: Seamless integration with existing workflow

## Architecture Summary

```
┌─────────────────────────────────────────────────────────────┐
│                    SupraFit Core (C++)                     │
├─────────────────────────────────────────────────────────────┤
│  Existing Components:                                       │
│  • MLFeatureExtractor (✅ Implemented)                     │
│  • Statistical Analysis (Monte Carlo, CV, etc.)            │
│  • CLI and GUI Infrastructure                               │
├─────────────────────────────────────────────────────────────┤
│  New ML Components (C++):                                  │
│  • NeuralNetwork (Inference Engine)                        │
│  • ModelPredictor (Integration Layer)                      │
│  • FeaturePreprocessor (Data Pipeline)                     │
│  • CLI Integration (--predict-models)                      │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│              Training Pipeline (Python - Optional)         │
├─────────────────────────────────────────────────────────────┤
│  • Automated Training Data Generation                      │
│  • PyTorch/TensorFlow Neural Network Training              │
│  • Hyperparameter Optimization                             │
│  • Model Export to SupraFit JSON Format                    │
└─────────────────────────────────────────────────────────────┘
```

## ML Pipeline Configuration

### `AddModels` vs. `MLModels`

- **`AddModels`:** This is the current standard key used in the input JSON to specify a list of models to be fitted to the data. It is used for both general-purpose model fitting and the ML pipeline.
- **`MLModels`:** This is a legacy key that was previously used to specify models for the ML pipeline. The `suprafit_cli` code is backward-compatible and will still recognize and use this key, but it is recommended to use `AddModels` for all new input JSON files.

### `ProcessMLPipeline` Key

The ML pipeline is activated by the presence of the `ProcessMLPipeline` key in the `Main` section of the input JSON file.

**Example:**

```json
{
    "Main": {
        "ProcessMLPipeline": true,
        ...
    },
    ...
}
```

Setting `ProcessMLPipeline` to `true` will trigger the ML pipeline workflow in `suprafit_cli`.

## Document Structure

### 1. [ML_INTEGRATION_ROADMAP.md](ML_INTEGRATION_ROADMAP.md)
Comprehensive project vision and strategy including:
- **Technical Architecture**: C++ inference engine with optional Python training
- **Neural Network Design**: Feature extraction, preprocessing, and prediction pipeline
- **Integration Strategy**: CLI, GUI, and existing SupraFit workflow integration
- **Training Data Strategy**: Automated generation of diverse, balanced datasets
- **Risk Assessment**: Technical, schedule, and quality risk mitigation

### 2. [TECHNICAL_ARCHITECTURE.md](TECHNICAL_ARCHITECTURE.md)
Detailed technical implementation covering:
- **Neural Network Engine**: Lightweight C++ implementation using Eigen
- **Model Predictor**: Confidence scoring and uncertainty quantification
- **Feature Pipeline**: Integration with existing MLFeatureExtractor
- **Performance Optimization**: Memory management, batch processing, parallel execution
- **Error Handling**: Robust validation and graceful fallback mechanisms

### 3. [TRAINING_PIPELINE.md](TRAINING_PIPELINE.md)
Python-based training system for model development:
- **Training Data Generation**: Automated SupraFit dataset creation with diverse parameters
- **Neural Network Training**: PyTorch implementation with hyperparameter optimization
- **Model Export**: Conversion to SupraFit-native JSON format
- **Validation Framework**: Comprehensive testing and accuracy benchmarking

### 4. [IMPLEMENTATION_PHASES.md](IMPLEMENTATION_PHASES.md)
Detailed development roadmap with timelines:
- **Phase 1**: Foundation Infrastructure (4-6 weeks)
- **Phase 2**: Training Pipeline and Model Development (3-4 weeks)  
- **Phase 3**: Advanced Features and GUI Integration (4-5 weeks)
- **Resource Requirements**: Team, hardware, and software dependencies
- **Success Metrics**: Technical and user experience validation criteria

### 5. [C++_EXAMPLES.md](C++_EXAMPLES.md)
Complete, production-ready code examples:
- **Neural Network Core**: Full implementation with Eigen matrix operations
- **Model Predictor**: Integration with SupraFit workflow and confidence scoring
- **Feature Preprocessor**: Transformation of MLFeatureExtractor output
- **CLI Integration**: Command-line interface for model prediction

## Current Status

### ✅ Completed (Foundation Ready)
- **MLFeatureExtractor**: Generates ML training data from fitted models
- **Statistical Analysis**: JSON-based statistical feature extraction
- **CLI Infrastructure**: Enhanced command-line interface with ProcessMLPipeline
- **File Format Support**: All SupraFit file types (.suprafit, .json, ML-Pipeline)
- **ML Pipeline Workflow**: Complete data generation → fitting → analysis pipeline

### 🚧 Planned (Architecture Defined, Implementation Pending)
1. **Neural Network Core** - Inference engine for model prediction
   - Status: Architecture designed, NOT YET IMPLEMENTED
   - Location: Planned for `src/ml/neural_network.cpp`
2. **Feature Preprocessing** - Data normalization and feature engineering
   - Status: Architecture designed, NOT YET IMPLEMENTED
   - Location: Planned for `src/ml/feature_preprocessor.cpp`
3. **Model Prediction** - Integration layer for neural network predictions
   - Status: Architecture designed, NOT YET IMPLEMENTED
   - Location: Planned for `src/ml/model_predictor.cpp`
4. **CLI Integration** - `--predict-models` option for automated model selection
   - Status: Architecture designed, NOT YET IMPLEMENTED
   - Enhancement to `suprafit_cli.cpp`

## Integration Benefits

### For Users
- **Automated Model Selection**: Eliminate manual model exploration
- **Confidence Scoring**: Quantified prediction reliability
- **Time Savings**: 30%+ reduction in analysis time
- **Educational Value**: Transparent reasoning for predictions

### For Developers  
- **No Runtime Dependencies**: Pure C++ with optional Python training
- **Modular Design**: Easy to extend and maintain
- **Performance Optimized**: <100ms prediction time, <100MB memory usage
- **Educational Focus**: Clear algorithms that don't obscure scientific content

## Getting Started

### For Implementation
1. **Review Architecture**: Start with `TECHNICAL_ARCHITECTURE.md`
2. **Examine Code Examples**: See `C++_EXAMPLES.md` for implementation details
3. **Follow Phase Plan**: Use `IMPLEMENTATION_PHASES.md` for development timeline

### For Training (Optional)
1. **Generate Training Data**: Use existing MLFeatureExtractor with diverse datasets
2. **Set Up Python Environment**: Install PyTorch and dependencies
3. **Follow Training Pipeline**: Implement scripts from `TRAINING_PIPELINE.md`

## Performance Targets

| Metric | Target | Current Status |
|--------|--------|----------------|
| Model Accuracy | >90% | Pending training |
| Inference Time | <100ms | Architecture ready |
| Memory Usage | <100MB | Architecture ready |
| Build Impact | <20% | CMake integration planned |
| User Adoption | >50% | UX design planned |

## Technical Dependencies

### Required (C++ Runtime)
- **Qt6**: Core, JSON handling (✅ Available)
- **Eigen**: Matrix operations (✅ Available)
- **CMake**: Build system (✅ Available)

### Optional (Python Training)
- **PyTorch/TensorFlow**: Neural network training
- **scikit-learn**: Data preprocessing and validation
- **NumPy/Pandas**: Data manipulation

## Risk Mitigation

### Technical Risks → Solutions
- **Model Accuracy**: Extensive validation with diverse datasets
- **Performance**: Profiling and optimization at each phase
- **Integration**: Incremental development with comprehensive testing

### Schedule Risks → Solutions
- **Dependencies**: Early resolution and fallback plans
- **Scope Creep**: Clear requirements and change control
- **Resource Constraints**: Parallel development and agile methodology

## Success Criteria

- ✅ **Model Accuracy**: >90% correct predictions on validation set
- ✅ **Performance**: <100ms inference time per prediction
- ✅ **Integration**: Seamless CLI and GUI workflow integration  
- ✅ **User Experience**: Intuitive interface with clear confidence indicators
- ✅ **Maintainability**: Clean, documented code following SupraFit patterns

This comprehensive plan provides a roadmap for successfully integrating machine learning into SupraFit while maintaining its core philosophy of educational transparency and C++ performance.