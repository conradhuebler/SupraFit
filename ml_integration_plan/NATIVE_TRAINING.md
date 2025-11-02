# SupraFit Native ML Training with GPU Support

## Overview

Extended SupraFit ML capabilities with native C++ training support and optional GPU acceleration. This enables complete machine learning workflows without external dependencies while providing high-performance training capabilities.

## Architecture Extension

```
┌─────────────────────────────────────────────────────────────┐
│                 SupraFit ML Core (C++)                     │
├─────────────────────────────────────────────────────────────┤
│  Inference Components:                                      │
│  • NeuralNetwork (CPU/GPU inference)                       │
│  • ModelPredictor (Integration layer)                      │
│  • FeaturePreprocessor (Data pipeline)                     │
├─────────────────────────────────────────────────────────────┤
│  Training Components (NEW):                                │
│  • NeuralNetworkTrainer (Backpropagation, optimizers)     │
│  • GPUManager (CUDA/OpenCL acceleration)                   │
│  • TrainingDataManager (Batch processing, augmentation)    │
│  • HyperparameterOptimizer (Grid/Bayesian optimization)    │
│  • TrainingPipeline (End-to-end training workflow)        │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│               Optional GPU Backends                        │
├─────────────────────────────────────────────────────────────┤
│  • CUDA (NVIDIA GPUs) - cuBLAS, cuDNN                     │
│  • OpenCL (AMD/Intel GPUs) - Cross-platform               │
│  • CPU Fallback (Eigen + OpenMP) - Always available       │
└─────────────────────────────────────────────────────────────┘
```

## Native Training Implementation

### 1. Neural Network Trainer Core

```cpp
// src/ml/neural_network_trainer.h
#pragma once

#include "neural_network.h"
#include <QtCore/QObject>
#include <QtCore/QElapsedTimer>
#include <memory>

class Optimizer {
public:
    virtual ~Optimizer() = default;
    virtual void step(NeuralNetwork* network, const std::vector<Eigen::MatrixXd>& gradients) = 0;
    virtual void reset() = 0;
    virtual QString name() const = 0;
};

class SGDOptimizer : public Optimizer {
public:
    SGDOptimizer(double learning_rate = 0.01, double momentum = 0.9, double weight_decay = 0.0);
    void step(NeuralNetwork* network, const std::vector<Eigen::MatrixXd>& gradients) override;
    void reset() override;
    QString name() const override { return "SGD"; }
    
private:
    double learning_rate_;
    double momentum_;
    double weight_decay_;
    std::vector<Eigen::MatrixXd> velocity_weights_;
    std::vector<Eigen::VectorXd> velocity_biases_;
};

class AdamOptimizer : public Optimizer {
public:
    AdamOptimizer(double learning_rate = 0.001, double beta1 = 0.9, double beta2 = 0.999, double epsilon = 1e-8);
    void step(NeuralNetwork* network, const std::vector<Eigen::MatrixXd>& gradients) override;
    void reset() override;
    QString name() const override { return "Adam"; }
    
private:
    double learning_rate_, beta1_, beta2_, epsilon_;
    std::vector<Eigen::MatrixXd> m_weights_, v_weights_;  // First and second moments for weights
    std::vector<Eigen::VectorXd> m_biases_, v_biases_;   // First and second moments for biases
    int timestep_;
};

class LossFunction {
public:
    virtual ~LossFunction() = default;
    virtual double computeLoss(const Eigen::MatrixXd& predictions, const Eigen::MatrixXd& targets) const = 0;
    virtual Eigen::MatrixXd computeGradient(const Eigen::MatrixXd& predictions, const Eigen::MatrixXd& targets) const = 0;
    virtual QString name() const = 0;
};

class CrossEntropyLoss : public LossFunction {
public:
    double computeLoss(const Eigen::MatrixXd& predictions, const Eigen::MatrixXd& targets) const override;
    Eigen::MatrixXd computeGradient(const Eigen::MatrixXd& predictions, const Eigen::MatrixXd& targets) const override;
    QString name() const override { return "CrossEntropy"; }
};

class MeanSquaredErrorLoss : public LossFunction {
public:
    double computeLoss(const Eigen::MatrixXd& predictions, const Eigen::MatrixXd& targets) const override;
    Eigen::MatrixXd computeGradient(const Eigen::MatrixXd& predictions, const Eigen::MatrixXd& targets) const override;
    QString name() const override { return "MSE"; }
};

class NeuralNetworkTrainer : public QObject {
    Q_OBJECT
    
public:
    struct TrainingConfig {
        // Basic training parameters
        int epochs = 1000;
        int batch_size = 32;
        double validation_split = 0.2;
        
        // Optimization
        QString optimizer = "Adam";  // "SGD", "Adam", "RMSprop"
        double learning_rate = 0.001;
        double momentum = 0.9;       // For SGD
        double weight_decay = 0.0;
        
        // Loss function
        QString loss_function = "CrossEntropy";  // "MSE", "CrossEntropy"
        
        // Regularization
        double dropout_rate = 0.3;
        double l1_regularization = 0.0;
        double l2_regularization = 0.0;
        
        // Early stopping
        bool early_stopping = true;
        int patience = 50;
        double min_delta = 1e-4;
        
        // Learning rate scheduling
        bool use_lr_scheduler = true;
        QString lr_scheduler = "ReduceLROnPlateau";  // "StepLR", "ExponentialLR"
        double lr_decay_factor = 0.5;
        int lr_decay_patience = 20;
        
        // GPU/Performance
        bool use_gpu = true;         // Auto-detect and use if available
        QString gpu_backend = "auto"; // "cuda", "opencl", "cpu", "auto"
        int num_threads = 0;         // 0 = auto-detect
        bool mixed_precision = false; // FP16 training (if supported)
        
        // Data augmentation
        bool enable_data_augmentation = false;
        double noise_factor = 0.05;
        double rotation_factor = 0.1;
        
        // Checkpointing
        bool save_checkpoints = true;
        int checkpoint_frequency = 100;  // Save every N epochs
        QString checkpoint_dir = "./checkpoints";
        
        // Logging and monitoring
        int log_frequency = 10;      // Log every N epochs
        bool save_training_history = true;
        bool plot_training_curves = false;
    };
    
    struct TrainingResult {
        bool success = false;
        QString error_message;
        
        // Training metrics
        QVector<double> train_loss_history;
        QVector<double> val_loss_history;
        QVector<double> train_accuracy_history;
        QVector<double> val_accuracy_history;
        QVector<double> learning_rate_history;
        
        // Best model information
        int best_epoch = -1;
        double best_val_loss = std::numeric_limits<double>::max();
        double best_val_accuracy = 0.0;
        
        // Training statistics
        double total_training_time_ms = 0.0;
        double average_epoch_time_ms = 0.0;
        int total_epochs_trained = 0;
        
        // Hardware utilization
        QString hardware_used;       // "CPU", "CUDA", "OpenCL"
        double peak_memory_usage_mb = 0.0;
        double gpu_utilization_percent = 0.0;
        
        // Model information
        int final_parameter_count = 0;
        QJsonObject final_model_info;
    };
    
    explicit NeuralNetworkTrainer(QObject* parent = nullptr);
    ~NeuralNetworkTrainer();
    
    // Training interface
    TrainingResult trainNetwork(NeuralNetwork* network,
                               const Eigen::MatrixXd& X_train,
                               const Eigen::MatrixXd& y_train,
                               const TrainingConfig& config = TrainingConfig());
    
    TrainingResult trainNetworkWithValidation(NeuralNetwork* network,
                                             const Eigen::MatrixXd& X_train,
                                             const Eigen::MatrixXd& y_train,
                                             const Eigen::MatrixXd& X_val,
                                             const Eigen::MatrixXd& y_val,
                                             const TrainingConfig& config = TrainingConfig());
    
    // Advanced training methods
    TrainingResult hyperparameterOptimization(NeuralNetwork* network,
                                             const Eigen::MatrixXd& X,
                                             const Eigen::MatrixXd& y,
                                             const QJsonObject& search_space);
    
    TrainingResult crossValidationTraining(NeuralNetwork* network,
                                          const Eigen::MatrixXd& X,
                                          const Eigen::MatrixXd& y,
                                          int k_folds = 5);
    
    // Transfer learning
    bool loadPretrainedWeights(NeuralNetwork* network, const QString& pretrained_model_path);
    TrainingResult fineTuning(NeuralNetwork* network,
                             const Eigen::MatrixXd& X,
                             const Eigen::MatrixXd& y,
                             const QStringList& layers_to_freeze = QStringList());
    
    // Hardware management
    bool initializeGPU();
    QStringList getAvailableGPUBackends();
    QString getCurrentBackend() const;
    QJsonObject getHardwareInfo() const;
    
    // Training state management
    bool saveCheckpoint(const NeuralNetwork* network, const QString& path, int epoch, double loss);
    bool loadCheckpoint(NeuralNetwork* network, const QString& path);
    
    // Monitoring and diagnostics
    QJsonObject getTrainingDiagnostics() const;
    void setProgressCallback(std::function<void(int, double, double)> callback);
    
signals:
    void trainingStarted(int total_epochs);
    void epochCompleted(int epoch, double train_loss, double val_loss, double val_accuracy);
    void trainingCompleted(const TrainingResult& result);
    void hardwareStatusChanged(const QString& backend, double utilization);
    void errorOccurred(const QString& error);
    
private slots:
    void handleGPUError(const QString& error);
    
private:
    // Core training implementation
    void backwardPass(NeuralNetwork* network, const Eigen::MatrixXd& predictions, 
                      const Eigen::MatrixXd& targets, std::vector<Eigen::MatrixXd>& gradients);
    
    // Batch processing
    std::pair<Eigen::MatrixXd, Eigen::MatrixXd> createMiniBatch(
        const Eigen::MatrixXd& X, const Eigen::MatrixXd& y, 
        const QVector<int>& indices, int batch_size);
    
    // Data augmentation
    Eigen::MatrixXd augmentData(const Eigen::MatrixXd& X, const TrainingConfig& config);
    
    // Validation and metrics
    double computeAccuracy(const Eigen::MatrixXd& predictions, const Eigen::MatrixXd& targets);
    QJsonObject computeDetailedMetrics(const Eigen::MatrixXd& predictions, const Eigen::MatrixXd& targets);
    
    // Learning rate scheduling
    void updateLearningRate(Optimizer* optimizer, const TrainingConfig& config, 
                           double current_val_loss, int epoch);
    
    // Memory management
    void optimizeMemoryUsage();
    double getCurrentMemoryUsage() const;
    
    // Components
    std::unique_ptr<class GPUManager> gpu_manager_;
    std::unique_ptr<Optimizer> optimizer_;
    std::unique_ptr<LossFunction> loss_function_;
    
    // Training state
    TrainingResult current_result_;
    QElapsedTimer training_timer_;
    std::function<void(int, double, double)> progress_callback_;
    
    // Configuration
    TrainingConfig current_config_;
    QString checkpoint_directory_;
};
```

### 2. GPU Management System

```cpp
// src/ml/gpu_manager.h
#pragma once

#include <QtCore/QObject>
#include <QtCore/QJsonObject>
#include <Eigen/Dense>
#include <memory>

enum class GPUBackend {
    CPU,
    CUDA,
    OpenCL,
    Auto
};

struct GPUInfo {
    QString name;
    QString vendor;
    size_t memory_total_mb;
    size_t memory_available_mb;
    int compute_units;
    int max_work_group_size;
    QString version;
    bool supports_fp16;
    bool supports_fp64;
    double performance_score;  // Relative performance metric
};

class GPUKernel {
public:
    virtual ~GPUKernel() = default;
    virtual bool initialize(const GPUInfo& gpu_info) = 0;
    virtual void cleanup() = 0;
    virtual QString name() const = 0;
};

class MatrixMultiplyKernel : public GPUKernel {
public:
    virtual void multiply(const Eigen::MatrixXd& A, const Eigen::MatrixXd& B, 
                         Eigen::MatrixXd& C) = 0;
    virtual void multiplyTransposed(const Eigen::MatrixXd& A, const Eigen::MatrixXd& B, 
                                   Eigen::MatrixXd& C, bool transpose_A = false, bool transpose_B = false) = 0;
};

class ActivationKernel : public GPUKernel {
public:
    virtual void relu(const Eigen::MatrixXd& input, Eigen::MatrixXd& output) = 0;
    virtual void reluDerivative(const Eigen::MatrixXd& input, Eigen::MatrixXd& output) = 0;
    virtual void sigmoid(const Eigen::MatrixXd& input, Eigen::MatrixXd& output) = 0;
    virtual void sigmoidDerivative(const Eigen::MatrixXd& input, Eigen::MatrixXd& output) = 0;
    virtual void softmax(const Eigen::MatrixXd& input, Eigen::MatrixXd& output) = 0;
};

class GPUManager : public QObject {
    Q_OBJECT
    
public:
    explicit GPUManager(QObject* parent = nullptr);
    ~GPUManager();
    
    // Initialization and detection
    bool initialize(GPUBackend preferred_backend = GPUBackend::Auto);
    void cleanup();
    
    // GPU detection and selection
    QVector<GPUInfo> detectAvailableGPUs();
    bool selectGPU(int gpu_index);
    bool selectBestGPU();
    
    // Current GPU information
    GPUInfo getCurrentGPU() const;
    GPUBackend getCurrentBackend() const;
    bool isGPUAvailable() const;
    
    // Memory management
    size_t getAvailableMemory() const;
    size_t getTotalMemory() const;
    double getMemoryUtilization() const;
    bool allocateMemory(size_t size_bytes);
    void freeMemory();
    
    // Performance monitoring
    double getGPUUtilization() const;
    double getMemoryBandwidth() const;
    QJsonObject getPerformanceStats() const;
    
    // Kernel management
    template<typename KernelType>
    std::shared_ptr<KernelType> getKernel();
    
    bool loadKernelFromSource(const QString& kernel_name, const QString& source_code);
    bool loadKernelFromFile(const QString& kernel_name, const QString& file_path);
    
    // Matrix operations (GPU-accelerated)
    void matrixMultiply(const Eigen::MatrixXd& A, const Eigen::MatrixXd& B, Eigen::MatrixXd& C);
    void matrixAdd(const Eigen::MatrixXd& A, const Eigen::MatrixXd& B, Eigen::MatrixXd& C);
    void matrixTranspose(const Eigen::MatrixXd& input, Eigen::MatrixXd& output);
    
    // Element-wise operations
    void applyActivation(const Eigen::MatrixXd& input, Eigen::MatrixXd& output, const QString& activation);
    void computeGradients(const Eigen::MatrixXd& input, const Eigen::MatrixXd& grad_output, 
                         Eigen::MatrixXd& grad_input, const QString& activation);
    
    // Batch operations
    void batchMatrixMultiply(const QVector<Eigen::MatrixXd>& A_batch,
                            const QVector<Eigen::MatrixXd>& B_batch,
                            QVector<Eigen::MatrixXd>& C_batch);
    
    // Data transfer
    bool copyToGPU(const Eigen::MatrixXd& host_matrix);
    bool copyFromGPU(Eigen::MatrixXd& host_matrix);
    void synchronize();  // Wait for all GPU operations to complete
    
    // Benchmarking
    double benchmarkMatrixMultiply(int matrix_size = 1024);
    double benchmarkActivations(int vector_size = 100000);
    QJsonObject runFullBenchmark();
    
    // Configuration
    void setPreferredPrecision(bool use_fp16);
    void setMaxMemoryUsage(size_t max_memory_mb);
    void enableProfiling(bool enable);
    
signals:
    void gpuStatusChanged(bool available);
    void memoryWarning(double utilization_percent);
    void performanceAlert(const QString& message);
    void errorOccurred(const QString& error);
    
private:
    // Backend-specific implementations
    std::unique_ptr<class CUDABackend> cuda_backend_;
    std::unique_ptr<class OpenCLBackend> opencl_backend_;
    std::unique_ptr<class CPUBackend> cpu_backend_;
    
    // Current state
    GPUBackend current_backend_;
    GPUInfo current_gpu_;
    QHash<QString, std::shared_ptr<GPUKernel>> kernels_;
    
    // Performance monitoring
    mutable QElapsedTimer performance_timer_;
    mutable QJsonObject performance_stats_;
    
    // Memory management
    size_t allocated_memory_;
    size_t max_memory_limit_;
    
    // Configuration
    bool use_fp16_;
    bool profiling_enabled_;
    
    // Helper methods
    GPUBackend detectBestBackend();
    void initializeKernels();
    void updatePerformanceStats();
};

#ifdef CUDA_SUPPORT_ENABLED
class CUDABackend {
public:
    bool initialize();
    void cleanup();
    QVector<GPUInfo> detectDevices();
    bool selectDevice(int device_id);
    
    // CUDA-specific operations
    void* allocateMemory(size_t size);
    void freeMemory(void* ptr);
    void copyToDevice(void* device_ptr, const void* host_ptr, size_t size);
    void copyFromDevice(void* host_ptr, const void* device_ptr, size_t size);
    void synchronize();
    
    // cuBLAS operations
    void gemmFloat(const float* A, const float* B, float* C, 
                   int m, int n, int k, float alpha = 1.0f, float beta = 0.0f);
    void gemmDouble(const double* A, const double* B, double* C, 
                    int m, int n, int k, double alpha = 1.0, double beta = 0.0);
    
private:
    void* cuda_context_;
    void* cublas_handle_;
    int current_device_;
    QVector<void*> allocated_pointers_;
};
#endif

#ifdef OPENCL_SUPPORT_ENABLED  
class OpenCLBackend {
public:
    bool initialize();
    void cleanup();
    QVector<GPUInfo> detectDevices();
    bool selectDevice(int device_id);
    
    // OpenCL-specific operations
    void* createBuffer(size_t size, bool read_only = false);
    void releaseBuffer(void* buffer);
    void writeBuffer(void* buffer, const void* host_ptr, size_t size);
    void readBuffer(void* buffer, void* host_ptr, size_t size);
    
    // Kernel execution
    bool compileKernel(const QString& source, const QString& kernel_name);
    void executeKernel(const QString& kernel_name, const QVector<void*>& args, 
                      int global_work_size, int local_work_size = 0);
    
private:
    void* opencl_context_;
    void* opencl_command_queue_;
    void* opencl_device_;
    QHash<QString, void*> compiled_kernels_;
    QVector<void*> allocated_buffers_;
};
#endif

class CPUBackend {
public:
    bool initialize();
    void cleanup();
    
    // Optimized CPU operations using Eigen + OpenMP
    void matrixMultiply(const Eigen::MatrixXd& A, const Eigen::MatrixXd& B, Eigen::MatrixXd& C);
    void applyReLU(const Eigen::MatrixXd& input, Eigen::MatrixXd& output);
    void applySigmoid(const Eigen::MatrixXd& input, Eigen::MatrixXd& output);
    void applySoftmax(const Eigen::MatrixXd& input, Eigen::MatrixXd& output);
    
    // Multi-threaded batch operations
    void batchMatrixMultiply(const QVector<Eigen::MatrixXd>& A_batch,
                            const QVector<Eigen::MatrixXd>& B_batch,
                            QVector<Eigen::MatrixXd>& C_batch,
                            int num_threads = 0);
    
private:
    int num_threads_;
    bool openmp_available_;
};
```

### 3. Training Pipeline Integration

```cpp
// src/ml/training_pipeline.h
#pragma once

#include "neural_network_trainer.h"
#include "../capabilities/mlfeatureextractor.h"
#include <QtCore/QObject>
#include <QtCore/QDir>

class TrainingPipeline : public QObject {
    Q_OBJECT
    
public:
    struct PipelineConfig {
        // Data generation
        QString suprafit_cli_path;
        QVector<int> model_ids_to_train = {1, 2, 3, 10, 11, 20};  // NMR, ITC, UV-Vis
        int samples_per_model = 1000;
        QString training_data_dir = "./training_data";
        bool regenerate_training_data = false;
        
        // Network architecture
        QVector<int> hidden_layer_sizes = {128, 64, 32};
        QString activation_function = "ReLU";
        double dropout_rate = 0.3;
        
        // Training configuration
        NeuralNetworkTrainer::TrainingConfig training_config;
        
        // Validation and testing
        double validation_split = 0.2;
        double test_split = 0.1;
        bool cross_validation = true;
        int cv_folds = 5;
        
        // Model export
        QString output_model_path = "./trained_model.json";
        bool export_onnx = false;
        QString onnx_output_path = "./trained_model.onnx";
        
        // Hyperparameter optimization
        bool enable_hyperparameter_search = false;
        QJsonObject hyperparameter_search_space;
        int max_trials = 50;
        
        // Hardware
        bool prefer_gpu = true;
        QString gpu_backend = "auto";  // "cuda", "opencl", "cpu", "auto"
        
        // Logging and monitoring
        QString log_directory = "./training_logs";
        bool save_training_plots = true;
        bool verbose_logging = true;
    };
    
    struct PipelineResult {
        bool success = false;
        QString error_message;
        
        // Training results
        NeuralNetworkTrainer::TrainingResult training_result;
        
        // Validation metrics
        double final_accuracy = 0.0;
        double cross_validation_accuracy = 0.0;
        QJsonObject detailed_metrics;
        QJsonObject confusion_matrix;
        
        // Model information
        QString final_model_path;
        int total_parameters = 0;
        double model_size_mb = 0.0;
        
        // Dataset statistics  
        int total_training_samples = 0;
        QHash<int, int> samples_per_model;  // model_id -> sample_count
        QJsonObject dataset_statistics;
        
        // Performance metrics
        double total_pipeline_time_ms = 0.0;
        double data_generation_time_ms = 0.0;
        double training_time_ms = 0.0;
        double validation_time_ms = 0.0;
        
        // Hardware utilization
        QString hardware_used;
        double peak_memory_usage_mb = 0.0;
        double average_gpu_utilization = 0.0;
    };
    
    explicit TrainingPipeline(QObject* parent = nullptr);
    ~TrainingPipeline();
    
    // Main pipeline execution
    PipelineResult runCompletePipeline(const PipelineConfig& config);
    
    // Individual pipeline stages
    bool generateTrainingData(const PipelineConfig& config);
    PipelineResult trainModel(const PipelineConfig& config);
    QJsonObject validateModel(const QString& model_path, const PipelineConfig& config);
    
    // Advanced training methods
    PipelineResult hyperparameterOptimization(const PipelineConfig& config);
    PipelineResult ensembleTraining(const PipelineConfig& config, int num_models = 5);
    
    // Data management
    QJsonObject loadTrainingDataset(const QString& dataset_path);
    bool preprocessDataset(QJsonObject& dataset, const PipelineConfig& config);
    QVector<QJsonObject> createDataSplits(const QJsonObject& dataset, 
                                          double val_split, double test_split);
    
    // Model utilities
    bool exportModelToSupraFitFormat(const NeuralNetwork* network, 
                                   const QString& output_path,
                                   const QJsonObject& metadata);
    bool validateExportedModel(const QString& model_path);
    
    // Monitoring and diagnostics
    QJsonObject getPipelineStatus() const;
    QStringList getAvailableGPUs() const;
    void enableAdvancedLogging(bool enable);
    
signals:
    void pipelineStarted(const PipelineConfig& config);
    void stageCompleted(const QString& stage_name, double progress_percent);
    void trainingProgress(int epoch, double loss, double accuracy);
    void pipelineCompleted(const PipelineResult& result);
    void errorOccurred(const QString& error);
    void statusMessage(const QString& message);
    
private slots:
    void handleTrainingProgress(int epoch, double train_loss, double val_loss, double val_accuracy);
    void handleHardwareStatusChange(const QString& backend, double utilization);
    
private:
    // Pipeline components
    std::unique_ptr<NeuralNetworkTrainer> trainer_;
    std::unique_ptr<MLFeatureExtractor> feature_extractor_;
    std::unique_ptr<class TrainingDataGenerator> data_generator_;
    
    // Current pipeline state
    PipelineConfig current_config_;
    PipelineResult current_result_;
    QElapsedTimer pipeline_timer_;
    
    // Data management
    QString current_dataset_path_;
    QJsonObject current_dataset_;
    
    // Logging
    QString log_directory_;
    std::unique_ptr<QFile> log_file_;
    bool verbose_logging_;
    
    // Helper methods
    bool setupDirectories(const PipelineConfig& config);
    void logMessage(const QString& message, const QString& level = "INFO");
    QJsonObject computeConfusionMatrix(const Eigen::MatrixXd& predictions, 
                                      const Eigen::MatrixXd& targets);
    void saveTra.json iningPlots(const NeuralNetworkTrainer::TrainingResult& result);
    
    // Data generation integration
    bool generateModelDataset(int model_id, int num_samples, const QString& output_dir);
    QJsonObject consolidateTrainingDatasets(const QStringList& dataset_files);
};

class TrainingDataGenerator {
public:
    TrainingDataGenerator(const QString& suprafit_cli_path);
    
    // Generate training data for specific models
    bool generateModelSamples(int model_id, int num_samples, const QString& output_dir);
    
    // Generate comprehensive training dataset
    bool generateFullDataset(const QVector<int>& model_ids, int samples_per_model, 
                           const QString& output_dir);
    
    // Data augmentation and balancing
    QJsonObject augmentDataset(const QJsonObject& dataset, double augmentation_factor = 0.2);
    QJsonObject balanceDataset(const QJsonObject& dataset);
    
    // Quality control
    bool validateDataset(const QJsonObject& dataset);
    QJsonObject getDatasetStatistics(const QJsonObject& dataset);
    
private:
    QString cli_path_;
    QStringList temp_files_;
    
    // Configuration generation
    QJsonObject createTrainingConfiguration(int model_id, int sample_index);
    QJsonObject createRandomParameters(int model_id);
    
    // File management
    bool runSupraFitCLI(const QString& config_file, const QString& output_prefix);
    bool extractFeatures(const QString& result_file, const QString& features_file);
    void cleanupTempFiles();
};
```

### 4. CLI Integration for Native Training

```cpp
// src/client/suprafit_cli.cpp - Native training commands
bool SupraFitCli::TrainMLModel() {
    std::cout << "🚀 Starting SupraFit Native ML Training Pipeline\n\n";
    
    // Configuration
    TrainingPipeline::PipelineConfig config;
    config.suprafit_cli_path = QCoreApplication::applicationFilePath();
    config.training_data_dir = "./ml_training_data";
    config.output_model_path = "./suprafit_trained_model.json";
    
    // Parse command line options for training configuration
    if (m_parser.isSet("ml-epochs")) {
        config.training_config.epochs = m_parser.value("ml-epochs").toInt();
    }
    
    if (m_parser.isSet("ml-batch-size")) {
        config.training_config.batch_size = m_parser.value("ml-batch-size").toInt();
    }
    
    if (m_parser.isSet("ml-learning-rate")) {
        config.training_config.learning_rate = m_parser.value("ml-learning-rate").toDouble();
    }
    
    if (m_parser.isSet("ml-use-gpu")) {
        config.training_config.use_gpu = true;
        std::cout << "🎮 GPU acceleration enabled\n";
    }
    
    if (m_parser.isSet("ml-gpu-backend")) {
        config.training_config.gpu_backend = m_parser.value("ml-gpu-backend");
    }
    
    if (m_parser.isSet("ml-models")) {
        QString models_str = m_parser.value("ml-models");
        QStringList model_strs = models_str.split(',');
        config.model_ids_to_train.clear();
        for (const QString& model_str : model_strs) {
            config.model_ids_to_train.append(model_str.toInt());
        }
    }
    
    if (m_parser.isSet("ml-samples-per-model")) {
        config.samples_per_model = m_parser.value("ml-samples-per-model").toInt();
    }
    
    // Initialize training pipeline
    TrainingPipeline pipeline;
    
    // Connect progress signals
    connect(&pipeline, &TrainingPipeline::stageCompleted, 
            [](const QString& stage, double progress) {
                std::cout << "✅ " << stage.toStdString() << " completed (" 
                          << progress << "%)\n";
            });
    
    connect(&pipeline, &TrainingPipeline::trainingProgress,
            [](int epoch, double loss, double accuracy) {
                if (epoch % 50 == 0) {  // Log every 50 epochs
                    std::cout << "🧠 Epoch " << epoch 
                              << " - Loss: " << std::fixed << std::setprecision(4) << loss
                              << " - Accuracy: " << (accuracy * 100) << "%\n";
                }
            });
    
    connect(&pipeline, &TrainingPipeline::statusMessage,
            [](const QString& message) {
                std::cout << "ℹ️  " << message.toStdString() << "\n";
            });
    
    // Run training pipeline
    TrainingPipeline::PipelineResult result = pipeline.runCompletePipeline(config);
    
    if (result.success) {
        std::cout << "\n🎉 Training completed successfully!\n";
        std::cout << "📊 Final Results:\n";
        std::cout << "   • Accuracy: " << (result.final_accuracy * 100) << "%\n";
        std::cout << "   • Cross-validation: " << (result.cross_validation_accuracy * 100) << "%\n";
        std::cout << "   • Training samples: " << result.total_training_samples << "\n";
        std::cout << "   • Model parameters: " << result.total_parameters << "\n";
        std::cout << "   • Model size: " << result.model_size_mb << " MB\n";
        std::cout << "   • Training time: " << (result.training_time_ms / 1000.0) << " seconds\n";
        std::cout << "   • Hardware used: " << result.hardware_used.toStdString() << "\n";
        
        if (result.average_gpu_utilization > 0) {
            std::cout << "   • GPU utilization: " << result.average_gpu_utilization << "%\n";
            std::cout << "   • Peak memory usage: " << result.peak_memory_usage_mb << " MB\n";
        }
        
        std::cout << "\n📁 Model saved to: " << result.final_model_path.toStdString() << "\n";
        std::cout << "🚀 Ready to use for model prediction!\n\n";
        
        // Optional: Run quick validation
        if (m_parser.isSet("ml-validate-after-training")) {
            std::cout << "🔍 Running model validation...\n";
            QJsonObject validation_result = pipeline.validateModel(result.final_model_path, config);
            
            if (validation_result.contains("accuracy")) {
                double val_accuracy = validation_result["accuracy"].toDouble();
                std::cout << "✅ Validation accuracy: " << (val_accuracy * 100) << "%\n";
            }
        }
        
    } else {
        std::cout << "\n❌ Training failed: " << result.error_message.toStdString() << "\n";
        return false;
    }
    
    return true;
}

bool SupraFitCli::BenchmarkGPU() {
    std::cout << "🎮 GPU Benchmark for SupraFit ML Training\n\n";
    
    GPUManager gpu_manager;
    
    if (!gpu_manager.initialize()) {
        std::cout << "❌ Failed to initialize GPU manager\n";
        return false;
    }
    
    // Detect available GPUs
    QVector<GPUInfo> gpus = gpu_manager.detectAvailableGPUs();
    
    std::cout << "🔍 Detected GPUs:\n";
    for (int i = 0; i < gpus.size(); ++i) {
        const GPUInfo& gpu = gpus[i];
        std::cout << "   " << (i + 1) << ". " << gpu.name.toStdString() 
                  << " (" << gpu.vendor.toStdString() << ")\n";
        std::cout << "      Memory: " << gpu.memory_total_mb << " MB\n";
        std::cout << "      Compute Units: " << gpu.compute_units << "\n";
        std::cout << "      Performance Score: " << gpu.performance_score << "\n";
        std::cout << "      FP16 Support: " << (gpu.supports_fp16 ? "Yes" : "No") << "\n\n";
    }
    
    if (gpus.isEmpty()) {
        std::cout << "⚠️  No compatible GPUs found. Training will use CPU.\n";
        return true;
    }
    
    // Select best GPU for benchmarking
    if (!gpu_manager.selectBestGPU()) {
        std::cout << "❌ Failed to select GPU\n";
        return false;
    }
    
    GPUInfo selected_gpu = gpu_manager.getCurrentGPU();
    std::cout << "🏆 Selected GPU: " << selected_gpu.name.toStdString() << "\n\n";
    
    // Run benchmarks
    std::cout << "🏃 Running benchmarks...\n";
    
    double matrix_mult_time = gpu_manager.benchmarkMatrixMultiply(1024);
    double activation_time = gpu_manager.benchmarkActivations(100000);
    
    std::cout << "📊 Benchmark Results:\n";
    std::cout << "   • Matrix Multiplication (1024x1024): " << matrix_mult_time << " ms\n";
    std::cout << "   • Activation Functions (100k elements): " << activation_time << " ms\n";
    
    QJsonObject full_benchmark = gpu_manager.runFullBenchmark();
    std::cout << "   • Memory Bandwidth: " 
              << full_benchmark["memory_bandwidth_gb_s"].toDouble() << " GB/s\n";
    std::cout << "   • FP32 Performance: " 
              << full_benchmark["fp32_gflops"].toDouble() << " GFLOPS\n";
    
    if (selected_gpu.supports_fp16) {
        std::cout << "   • FP16 Performance: " 
                  << full_benchmark["fp16_gflops"].toDouble() << " GFLOPS\n";
    }
    
    // Estimate training performance
    double estimated_speedup = full_benchmark["estimated_training_speedup"].toDouble();
    std::cout << "\n🚀 Estimated training speedup vs CPU: " << estimated_speedup << "x\n";
    
    if (estimated_speedup > 5.0) {
        std::cout << "✅ Excellent GPU performance for ML training!\n";
    } else if (estimated_speedup > 2.0) {
        std::cout << "👍 Good GPU performance for ML training\n";
    } else {
        std::cout << "⚠️  Limited GPU speedup - CPU training may be competitive\n";
    }
    
    return true;
}
```

### 5. CMake Integration for Native Training + GPU Support

```cmake
# CMakeLists.txt - ML Training and GPU support additions
option(BUILD_ML_TRAINING "Build with native ML training support" ON)
option(ENABLE_GPU_SUPPORT "Enable GPU acceleration for ML training" ON)
option(ENABLE_CUDA_SUPPORT "Enable NVIDIA CUDA support" AUTO)
option(ENABLE_OPENCL_SUPPORT "Enable OpenCL support" AUTO)

if(BUILD_ML_TRAINING)
    message(STATUS "Building with native ML training support")
    
    # ML Training source files
    set(ml_training_SRC
        src/ml/neural_network_trainer.cpp
        src/ml/training_pipeline.cpp
        src/ml/optimizers.cpp
        src/ml/loss_functions.cpp
        src/ml/data_augmentation.cpp
    )
    
    # GPU Support
    if(ENABLE_GPU_SUPPORT)
        list(APPEND ml_training_SRC src/ml/gpu_manager.cpp)
        
        # CUDA Support
        if(ENABLE_CUDA_SUPPORT)
            find_package(CUDAToolkit QUIET)
            if(CUDAToolkit_FOUND)
                enable_language(CUDA)
                list(APPEND ml_training_SRC 
                     src/ml/cuda_backend.cpp
                     src/ml/cuda_kernels.cu)
                target_compile_definitions(core PRIVATE CUDA_SUPPORT_ENABLED)
                target_link_libraries(core CUDA::cudart CUDA::cublas CUDA::curand)
                message(STATUS "CUDA support enabled - Version ${CUDAToolkit_VERSION}")
                
                # cuDNN for optimized neural network operations
                find_package(CUDNN QUIET)
                if(CUDNN_FOUND)
                    target_link_libraries(core ${CUDNN_LIBRARIES})
                    target_include_directories(core PRIVATE ${CUDNN_INCLUDE_DIRS})
                    target_compile_definitions(core PRIVATE CUDNN_SUPPORT_ENABLED)
                    message(STATUS "cuDNN support enabled")
                endif()
            else()
                message(WARNING "CUDA requested but not found")
                set(ENABLE_CUDA_SUPPORT OFF)
            endif()
        endif()
        
        # OpenCL Support  
        if(ENABLE_OPENCL_SUPPORT)
            find_package(OpenCL QUIET)
            if(OpenCL_FOUND)
                list(APPEND ml_training_SRC 
                     src/ml/opencl_backend.cpp
                     src/ml/opencl_kernels.cpp)
                target_link_libraries(core OpenCL::OpenCL)
                target_compile_definitions(core PRIVATE OPENCL_SUPPORT_ENABLED)
                message(STATUS "OpenCL support enabled - Version ${OpenCL_VERSION_STRING}")
                
                # Copy OpenCL kernel files to build directory
                file(COPY src/ml/kernels/ DESTINATION ${CMAKE_BINARY_DIR}/kernels/)
            else()
                message(WARNING "OpenCL requested but not found")
                set(ENABLE_OPENCL_SUPPORT OFF)
            endif()
        endif()
        
        # CPU Backend (always available as fallback)
        list(APPEND ml_training_SRC src/ml/cpu_backend.cpp)
        
        # OpenMP for CPU parallelization
        find_package(OpenMP QUIET)
        if(OpenMP_CXX_FOUND)
            target_link_libraries(core OpenMP::OpenMP_CXX)
            target_compile_definitions(core PRIVATE OPENMP_SUPPORT_ENABLED)
            message(STATUS "OpenMP support enabled for CPU parallelization")
        endif()
        
        target_compile_definitions(core PRIVATE GPU_SUPPORT_ENABLED)
        message(STATUS "GPU support enabled")
        
    else()
        message(STATUS "GPU support disabled - CPU-only training")
    endif()
    
    # Add ML training sources to core library
    target_sources(core PRIVATE ${ml_training_SRC})
    target_compile_definitions(core PRIVATE ML_TRAINING_ENABLED)
    
    # Additional dependencies for training
    find_package(Threads REQUIRED)
    target_link_libraries(core Threads::Threads)
    
    # Optional: Intel MKL for optimized CPU operations
    find_package(MKL QUIET)
    if(MKL_FOUND)
        target_link_libraries(core ${MKL_LIBRARIES})
        target_include_directories(core PRIVATE ${MKL_INCLUDE_DIRS})
        target_compile_definitions(core PRIVATE MKL_SUPPORT_ENABLED)
        message(STATUS "Intel MKL support enabled for optimized CPU operations")
    endif()
    
    # CLI commands for training
    target_compile_definitions(suprafit_cli PRIVATE ML_TRAINING_ENABLED)
    if(ENABLE_GPU_SUPPORT)
        target_compile_definitions(suprafit_cli PRIVATE GPU_SUPPORT_ENABLED)
    endif()
    
else()
    message(STATUS "Native ML training support disabled")
endif()

# Installation
if(BUILD_ML_TRAINING)
    # Install OpenCL kernels
    if(ENABLE_OPENCL_SUPPORT)
        install(DIRECTORY src/ml/kernels/
                DESTINATION share/suprafit/opencl_kernels
                FILES_MATCHING PATTERN "*.cl")
    endif()
    
    # Install default training configurations
    install(FILES ml_training_configs/default_training_config.json
                  ml_training_configs/gpu_optimized_config.json
            DESTINATION share/suprafit/ml_configs)
endif()

# Platform-specific optimizations
if(WIN32 AND ENABLE_GPU_SUPPORT)
    # Windows-specific GPU library paths
    if(ENABLE_CUDA_SUPPORT)
        set(CUDA_TOOLKIT_ROOT_DIR "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v12.0")
    endif()
elseif(APPLE AND ENABLE_GPU_SUPPORT)
    # macOS Metal Performance Shaders (future enhancement)
    find_library(METAL_FRAMEWORK Metal)
    if(METAL_FRAMEWORK)
        target_link_libraries(core ${METAL_FRAMEWORK})
        message(STATUS "Metal framework found - GPU compute available")
    endif()
endif()
```

### 6. Usage Examples

```bash
# Native SupraFit ML Training Examples

# Basic training with CPU
./suprafit_cli --train-ml-model \
    --ml-models "1,2,3,10,11" \
    --ml-samples-per-model 2000 \
    --ml-epochs 1000 \
    --ml-batch-size 64

# GPU-accelerated training
./suprafit_cli --train-ml-model \
    --ml-use-gpu \
    --ml-gpu-backend cuda \
    --ml-models "1,2,3,10,11,20" \
    --ml-samples-per-model 5000 \
    --ml-epochs 2000 \
    --ml-learning-rate 0.001 \
    --ml-validate-after-training

# GPU benchmark before training
./suprafit_cli --benchmark-gpu

# Hyperparameter optimization
./suprafit_cli --train-ml-model \
    --ml-hyperparameter-optimization \
    --ml-max-trials 100 \
    --ml-use-gpu

# Cross-validation training
./suprafit_cli --train-ml-model \
    --ml-cross-validation \
    --ml-cv-folds 5 \
    --ml-models "1,2,3" \
    --ml-samples-per-model 3000

# Export trained model to different formats
./suprafit_cli --export-trained-model \
    --input-model trained_model.json \
    --export-onnx \
    --output-onnx model.onnx

# Training with custom configuration
./suprafit_cli --train-ml-model \
    --ml-config custom_training_config.json \
    --ml-use-gpu \
    --ml-gpu-backend auto
```

### 7. Performance Benefits

**GPU Training Speedup (estimated):**
- **NVIDIA RTX 4090**: 15-25x speedup over CPU
- **NVIDIA RTX 3080**: 8-12x speedup over CPU  
- **AMD RX 7800 XT**: 6-10x speedup over CPU
- **Intel Arc A770**: 4-8x speedup over CPU

**Memory Requirements:**
- **CPU Training**: 4-8GB RAM
- **GPU Training**: 6-12GB VRAM + 4GB RAM
- **Large Models**: Up to 16GB VRAM for complex architectures

**Training Time Examples:**
- **10,000 samples, CPU**: ~2-4 hours
- **10,000 samples, GPU**: ~15-30 minutes  
- **100,000 samples, CPU**: ~20-40 hours
- **100,000 samples, GPU**: ~2-4 hours

This native training implementation provides SupraFit with complete machine learning autonomy while maintaining its educational-first philosophy and offering significant performance advantages through optional GPU acceleration.