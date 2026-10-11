# SupraFit ML Training Pipeline

## Python Training Environment (Optional Component)

### Overview
While SupraFit's neural network inference runs entirely in C++, the training pipeline uses Python for maximum flexibility and access to mature ML frameworks. This separation ensures no Python runtime dependencies for end users.

### Training Architecture

```
Training Pipeline:
┌─────────────────────┐    ┌──────────────────┐    ┌─────────────────┐
│ SupraFit Data       │───▶│ Python Training  │───▶│ C++ Model       │
│ Generation & Export │    │ (PyTorch/TF)     │    │ (JSON/ONNX)     │
└─────────────────────┘    └──────────────────┘    └─────────────────┘
         │                            │                        │
         ▼                            ▼                        ▼
  MLFeatureExtractor          Neural Network              NeuralNetwork
  Batch Generation            Hyperparameter              ModelPredictor
  JSON Training Data          Optimization                C++ Inference
```

## Training Data Generation

### 1. Comprehensive Dataset Creation

```python
#!/usr/bin/env python3
"""
training/generate_training_data.py
Automated training data generation for SupraFit ML models
"""

import subprocess
import json
import os
import numpy as np
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor, ProcessPoolExecutor
import argparse

class SupraFitTrainingDataGenerator:
    def __init__(self, suprafit_cli_path, output_dir):
        self.cli_path = Path(suprafit_cli_path)
        self.output_dir = Path(output_dir)
        self.output_dir.mkdir(parents=True, exist_ok=True)
        
    def generate_model_configurations(self, model_id, samples_per_model):
        """Generate diverse configurations for a specific model"""
        configs = []
        
        # Parameter ranges for different model types
        model_configs = {
            1: {  # NMR 1:1
                "global_limits": ["[1 6]", "[2 8]", "[0.5 10]"],
                "local_limits": [
                    "[6.0 7.0; 5.5 6.5; 2.0 3.0; 1.8 2.8]",
                    "[6.5 7.5; 6.0 7.0; 2.2 3.2; 2.0 3.0]",
                    "[5.8 6.8; 5.3 6.3; 1.9 2.9; 1.7 2.7]"
                ],
                "series_range": [2, 4],
                "datapoints_range": [8, 50]
            },
            2: {  # NMR 1:2
                "global_limits": ["[1 6]", "[3 10]", "[0.8 8]"],
                "local_limits": [
                    "[6.0 7.0; 5.5 6.5; 4.5 5.5; 2.0 3.0; 1.8 2.8]",
                    "[6.2 7.2; 5.7 6.7; 4.7 5.7; 2.1 3.1; 1.9 2.9]"
                ],
                "series_range": [3, 5], 
                "datapoints_range": [12, 60]
            },
            # Add configurations for other model types...
        }
        
        if model_id not in model_configs:
            raise ValueError(f"Unknown model ID: {model_id}")
            
        config = model_configs[model_id]
        
        for i in range(samples_per_model):
            # Random parameter selection
            global_limit = np.random.choice(config["global_limits"])
            local_limit = np.random.choice(config["local_limits"])
            series = np.random.randint(*config["series_range"])
            datapoints = np.random.randint(*config["datapoints_range"])
            
            # Noise configuration
            noise_std = np.random.uniform(0.0001, 0.01, series).tolist()
            noise_seed = np.random.randint(1, 100000)
            
            sample_config = {
                "Main": {
                    "ProcessMLPipeline": True,
                    "FitModels": True,
                    "PostFitAnalysis": True,
                    "UseModularStructure": True,
                    "OutFile": f"training_model{model_id}_sample{i:04d}",
                    "Repeat": 1
                },
                "Independent": {
                    "Source": "generator",
                    "Generator": {
                        "Type": "equations",
                        "DataPoints": datapoints,
                        "Variables": 2,
                        "Equations": "0.001|(X - 1) * 0.0001"
                    }
                },
                "Dependent": {
                    "Source": "generator",
                    "Generator": {
                        "Type": "model", 
                        "Series": series,
                        "Model": {"ID": model_id},
                        "GlobalRandomLimits": global_limit,
                        "LocalRandomLimits": local_limit
                    },
                    "Noise": {
                        "Type": "gaussian",
                        "Std": noise_std,
                        "RandomSeed": noise_seed
                    }
                },
                "AddModels": {
                    f"model_{model_id}": {
                        "ID": model_id,
                        "Options": {"FastMode": True, "Convergency": 1e-7}
                    }
                },
                "PostFitAnalysis": {
                    "methods": [
                        {"Method": 1, "MaxSteps": 1000, "VarianceSource": 2},
                        {"Method": 4, "CVType": 1, "MaxSteps": 100}
                    ]
                }
            }
            
            configs.append((sample_config, f"training_model{model_id}_sample{i:04d}"))
            
        return configs
    
    def generate_single_sample(self, args):
        """Generate a single training sample (for parallel processing)"""
        config, sample_name = args
        config_path = self.output_dir / f"{sample_name}.json"
        
        try:
            # Write configuration
            with open(config_path, 'w') as f:
                json.dump(config, f, indent=2)
            
            # Run SupraFit CLI
            cmd = [str(self.cli_path), "-i", str(config_path)]
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=300)
            
            if result.returncode != 0:
                return {"success": False, "sample": sample_name, 
                       "error": result.stderr}
            
            # Extract ML features
            output_file = self.output_dir / f"{sample_name}-0.json" 
            if output_file.exists():
                ml_cmd = [str(self.cli_path), "--export-ml-training", 
                         "-i", str(output_file),
                         "--ml-output", str(self.output_dir / f"{sample_name}_features.json")]
                
                ml_result = subprocess.run(ml_cmd, capture_output=True, text=True, timeout=60)
                
                if ml_result.returncode == 0:
                    return {"success": True, "sample": sample_name, 
                           "features_file": f"{sample_name}_features.json"}
                else:
                    return {"success": False, "sample": sample_name,
                           "error": f"Feature extraction failed: {ml_result.stderr}"}
            else:
                return {"success": False, "sample": sample_name,
                       "error": "Output file not generated"}
                
        except Exception as e:
            return {"success": False, "sample": sample_name, "error": str(e)}
        finally:
            # Clean up config file
            if config_path.exists():
                config_path.unlink()
    
    def generate_full_dataset(self, model_ids, samples_per_model=1000, max_workers=4):
        """Generate complete training dataset"""
        all_tasks = []
        
        print(f"Generating training data for models: {model_ids}")
        print(f"Samples per model: {samples_per_model}")
        print(f"Total samples: {len(model_ids) * samples_per_model}")
        
        # Prepare all generation tasks
        for model_id in model_ids:
            configs = self.generate_model_configurations(model_id, samples_per_model)
            all_tasks.extend(configs)
        
        # Execute in parallel
        successful_samples = []
        failed_samples = []
        
        with ProcessPoolExecutor(max_workers=max_workers) as executor:
            results = list(executor.map(self.generate_single_sample, all_tasks))
        
        # Process results
        for result in results:
            if result["success"]:
                successful_samples.append(result)
            else:
                failed_samples.append(result)
        
        print(f"\nGeneration complete:")
        print(f"Successful samples: {len(successful_samples)}")
        print(f"Failed samples: {len(failed_samples)}")
        
        if failed_samples:
            print("\nFailed samples:")
            for failed in failed_samples[:10]:  # Show first 10 failures
                print(f"  {failed['sample']}: {failed['error']}")
            
        return successful_samples, failed_samples
    
    def consolidate_features(self, successful_samples):
        """Consolidate all feature files into training dataset"""
        consolidated_data = {
            "ml_training_metadata": {
                "version": "batch_generated_v1.0",
                "generated": "auto",
                "sample_count": len(successful_samples),
                "features": ["fit_quality_metrics", "statistical_analysis", "input_noise"]
            },
            "training_samples": []
        }
        
        for sample in successful_samples:
            features_file = self.output_dir / sample["features_file"]
            if features_file.exists():
                try:
                    with open(features_file, 'r') as f:
                        sample_data = json.load(f)
                    
                    if "training_samples" in sample_data:
                        consolidated_data["training_samples"].extend(
                            sample_data["training_samples"])
                    
                    # Clean up individual feature file
                    features_file.unlink()
                    
                except Exception as e:
                    print(f"Error processing {features_file}: {e}")
        
        # Save consolidated dataset
        output_path = self.output_dir / "consolidated_training_data.json"
        with open(output_path, 'w') as f:
            json.dump(consolidated_data, f, indent=2)
        
        print(f"\nConsolidated training data saved to: {output_path}")
        print(f"Total training samples: {len(consolidated_data['training_samples'])}")
        
        return output_path

def main():
    parser = argparse.ArgumentParser(description='Generate SupraFit ML training data')
    parser.add_argument('--cli-path', required=True, help='Path to suprafit_cli executable')
    parser.add_argument('--output-dir', required=True, help='Output directory for training data')
    parser.add_argument('--models', default='1,2,3', help='Comma-separated model IDs')
    parser.add_argument('--samples-per-model', type=int, default=1000, 
                       help='Number of samples per model')
    parser.add_argument('--workers', type=int, default=4, 
                       help='Number of parallel workers')
    
    args = parser.parse_args()
    
    model_ids = [int(x.strip()) for x in args.models.split(',')]
    
    generator = SupraFitTrainingDataGenerator(args.cli_path, args.output_dir)
    successful, failed = generator.generate_full_dataset(
        model_ids, args.samples_per_model, args.workers)
    
    if successful:
        consolidated_path = generator.consolidate_features(successful)
        print(f"\nTraining data generation complete!")
        print(f"Run: python train_model.py --input {consolidated_path}")

if __name__ == "__main__":
    main()
```

### 2. Usage Example

```bash
# Generate comprehensive training dataset
python training/generate_training_data.py \
    --cli-path ../build/release/bin/linux/suprafit_cli \
    --output-dir training_data/ \
    --models "1,2,3,10,11,20" \
    --samples-per-model 2000 \
    --workers 8

# Expected output:
# - 12,000 training samples total
# - Balanced across 6 model types
# - Diverse parameter ranges and noise levels
# - consolidated_training_data.json for Python training
```

## Neural Network Training

### 1. PyTorch Training Implementation

```python
#!/usr/bin/env python3
"""
training/train_model.py
PyTorch-based neural network training for SupraFit model selection
"""

import torch
import torch.nn as nn
import torch.optim as optim
import json
import numpy as np
from pathlib import Path
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import StandardScaler, LabelEncoder
from sklearn.metrics import classification_report, confusion_matrix
import argparse
import matplotlib.pyplot as plt
import seaborn as sns

class SupraFitDataset(torch.utils.data.Dataset):
    def __init__(self, features, labels):
        self.features = torch.FloatTensor(features)
        self.labels = torch.LongTensor(labels)
    
    def __len__(self):
        return len(self.features)
    
    def __getitem__(self, idx):
        return self.features[idx], self.labels[idx]

class SupraFitNN(nn.Module):
    def __init__(self, input_size, hidden_sizes, num_classes, dropout_rate=0.3):
        super(SupraFitNN, self).__init__()
        
        layers = []
        prev_size = input_size
        
        # Hidden layers
        for hidden_size in hidden_sizes:
            layers.append(nn.Linear(prev_size, hidden_size))
            layers.append(nn.ReLU())
            layers.append(nn.BatchNorm1d(hidden_size))
            layers.append(nn.Dropout(dropout_rate))
            prev_size = hidden_size
        
        # Output layer
        layers.append(nn.Linear(prev_size, num_classes))
        
        self.network = nn.Sequential(*layers)
        
        # Initialize weights
        self.apply(self._init_weights)
    
    def _init_weights(self, module):
        if isinstance(module, nn.Linear):
            nn.init.xavier_uniform_(module.weight)
            nn.init.constant_(module.bias, 0)
    
    def forward(self, x):
        return self.network(x)

class ModelTrainer:
    def __init__(self, config):
        self.config = config
        self.device = torch.device('cuda' if torch.cuda.is_available() else 'cpu')
        print(f"Using device: {self.device}")
        
        # Initialize components
        self.model = None
        self.scaler = StandardScaler()
        self.label_encoder = LabelEncoder()
        self.feature_names = None
        
    def load_and_preprocess_data(self, data_path):
        """Load and preprocess training data"""
        print(f"Loading training data from: {data_path}")
        
        with open(data_path, 'r') as f:
            data = json.load(f)
        
        training_samples = data['training_samples']
        print(f"Loaded {len(training_samples)} training samples")
        
        # Extract features and labels
        features_list = []
        labels_list = []
        
        for sample in training_samples:
            # Extract numerical features
            features = self.extract_features(sample)
            label = sample['label']['correct_model_id']
            
            features_list.append(features)
            labels_list.append(label)
        
        # Convert to numpy arrays
        X = np.array(features_list)
        y = np.array(labels_list)
        
        print(f"Feature matrix shape: {X.shape}")
        print(f"Label distribution: {np.bincount(y)}")
        
        # Preprocessing
        X = self.scaler.fit_transform(X)
        y = self.label_encoder.fit_transform(y)
        
        return X, y
    
    def extract_features(self, sample):
        """Extract numerical features from ML training sample"""
        features = []
        
        # Ground truth features
        gt = sample['ground_truth']
        features.extend([
            gt.get('datapoints', 0),
            gt.get('series_count', 0),
            len(gt.get('global_params', [])),
            len(gt.get('local_params', [])),
        ])
        
        # Add parameter values (flattened)
        global_params = gt.get('global_params', [])
        features.extend(global_params[:5])  # Limit to first 5
        features.extend([0] * (5 - len(global_params)))  # Pad if needed
        
        # Fit quality features
        for candidate in sample.get('candidate_models', [])[:1]:  # First candidate only
            fq = candidate.get('fit_quality', {})
            features.extend([
                fq.get('aic', 0),
                fq.get('aicc', 0), 
                fq.get('r_squared', 0),
                fq.get('sse', 0),
                fq.get('rmse', 0),
                fq.get('chi_squared', 0),
                fq.get('parameter_count', 0),
                fq.get('data_points', 0),
            ])
            
            # Statistical features (Monte Carlo)
            sf = candidate.get('statistical_features', {})
            mc = sf.get('monte_carlo', {})
            
            # Extract first parameter's MC statistics
            for key in mc:
                if key.startswith('param_'):
                    param_stats = mc[key]
                    features.extend([
                        param_stats.get('boxplot_mean', 0),
                        param_stats.get('boxplot_stddev', 0),
                        param_stats.get('confidence_interval_lower', 0),
                        param_stats.get('confidence_interval_upper', 0),
                    ])
                    break
            else:
                features.extend([0, 0, 0, 0])  # No MC data
        
        # Input noise features
        noise = gt.get('input_noise', {})
        features.extend([
            noise.get('RandomSeed', 0) / 100000,  # Normalize
            len(noise.get('Std', [])),
            np.mean(noise.get('Std', [0])),
        ])
        
        return features
    
    def create_model(self, input_size, num_classes):
        """Create neural network model"""
        hidden_sizes = self.config['hidden_sizes']
        dropout_rate = self.config['dropout_rate']
        
        model = SupraFitNN(input_size, hidden_sizes, num_classes, dropout_rate)
        return model.to(self.device)
    
    def train_model(self, X_train, y_train, X_val, y_val):
        """Train the neural network"""
        num_classes = len(np.unique(y_train))
        input_size = X_train.shape[1]
        
        print(f"Training model: {input_size} inputs -> {num_classes} classes")
        
        # Create model
        self.model = self.create_model(input_size, num_classes)
        
        # Loss and optimizer
        criterion = nn.CrossEntropyLoss()
        optimizer = optim.Adam(self.model.parameters(), 
                             lr=self.config['learning_rate'],
                             weight_decay=self.config['weight_decay'])
        
        # Learning rate scheduler
        scheduler = optim.lr_scheduler.ReduceLROnPlateau(
            optimizer, mode='min', factor=0.5, patience=10, verbose=True)
        
        # Data loaders
        train_dataset = SupraFitDataset(X_train, y_train)
        val_dataset = SupraFitDataset(X_val, y_val)
        
        train_loader = torch.utils.data.DataLoader(
            train_dataset, batch_size=self.config['batch_size'], shuffle=True)
        val_loader = torch.utils.data.DataLoader(
            val_dataset, batch_size=self.config['batch_size'], shuffle=False)
        
        # Training loop
        train_losses = []
        val_losses = []
        val_accuracies = []
        best_val_loss = float('inf')
        patience_counter = 0
        
        for epoch in range(self.config['epochs']):
            # Training
            self.model.train()
            train_loss = 0
            for features, labels in train_loader:
                features, labels = features.to(self.device), labels.to(self.device)
                
                optimizer.zero_grad()
                outputs = self.model(features)
                loss = criterion(outputs, labels)
                loss.backward()
                optimizer.step()
                
                train_loss += loss.item()
            
            # Validation
            self.model.eval()
            val_loss = 0
            correct = 0
            total = 0
            
            with torch.no_grad():
                for features, labels in val_loader:
                    features, labels = features.to(self.device), labels.to(self.device)
                    outputs = self.model(features)
                    loss = criterion(outputs, labels)
                    val_loss += loss.item()
                    
                    _, predicted = torch.max(outputs.data, 1)
                    total += labels.size(0)
                    correct += (predicted == labels).sum().item()
            
            train_loss /= len(train_loader)
            val_loss /= len(val_loader)
            val_accuracy = 100 * correct / total
            
            train_losses.append(train_loss)
            val_losses.append(val_loss)
            val_accuracies.append(val_accuracy)
            
            # Learning rate scheduling
            scheduler.step(val_loss)
            
            # Early stopping
            if val_loss < best_val_loss:
                best_val_loss = val_loss
                patience_counter = 0
                # Save best model
                torch.save(self.model.state_dict(), 'best_model.pth')
            else:
                patience_counter += 1
            
            if epoch % 10 == 0:
                print(f'Epoch [{epoch+1}/{self.config["epochs"]}], '
                      f'Train Loss: {train_loss:.4f}, '
                      f'Val Loss: {val_loss:.4f}, '
                      f'Val Acc: {val_accuracy:.2f}%')
            
            if patience_counter >= self.config['early_stopping_patience']:
                print(f'Early stopping at epoch {epoch+1}')
                break
        
        # Load best model
        self.model.load_state_dict(torch.load('best_model.pth'))
        
        return {
            'train_losses': train_losses,
            'val_losses': val_losses, 
            'val_accuracies': val_accuracies,
            'best_val_accuracy': max(val_accuracies)
        }
    
    def evaluate_model(self, X_test, y_test):
        """Evaluate model on test set"""
        test_dataset = SupraFitDataset(X_test, y_test)
        test_loader = torch.utils.data.DataLoader(
            test_dataset, batch_size=self.config['batch_size'], shuffle=False)
        
        self.model.eval()
        y_pred = []
        y_true = []
        
        with torch.no_grad():
            for features, labels in test_loader:
                features = features.to(self.device)
                outputs = self.model(features)
                _, predicted = torch.max(outputs, 1)
                
                y_pred.extend(predicted.cpu().numpy())
                y_true.extend(labels.numpy())
        
        # Convert back to original labels
        y_true_orig = self.label_encoder.inverse_transform(y_true)
        y_pred_orig = self.label_encoder.inverse_transform(y_pred)
        
        # Classification report
        report = classification_report(y_true_orig, y_pred_orig, output_dict=True)
        
        # Confusion matrix
        cm = confusion_matrix(y_true_orig, y_pred_orig)
        
        return {
            'classification_report': report,
            'confusion_matrix': cm.tolist(),
            'accuracy': report['accuracy'],
            'y_true': y_true_orig.tolist(),
            'y_pred': y_pred_orig.tolist()
        }
    
    def export_to_suprafit_format(self, output_path):
        """Export trained model to SupraFit JSON format"""
        # Extract model parameters
        model_data = {
            "model_metadata": {
                "version": "1.0",
                "created": "2025-01-15T10:30:00Z",
                "architecture": "feedforward", 
                "input_size": self.model.network[0].in_features,
                "output_size": self.model.network[-1].out_features,
                "training_samples": len(self.scaler.mean_),
                "model_type": "classification"
            },
            "feature_config": {
                "means": self.scaler.mean_.tolist(),
                "stds": self.scaler.scale_.tolist(),
                "feature_count": len(self.scaler.mean_)
            },
            "network_architecture": {
                "layers": []
            },
            "class_labels": {}
        }
        
        # Extract layer parameters
        layer_idx = 0
        for i, module in enumerate(self.model.network):
            if isinstance(module, nn.Linear):
                layer_data = {
                    "type": "dense",
                    "input_size": module.in_features,
                    "output_size": module.out_features,
                    "weights": module.weight.detach().cpu().numpy().tolist(),
                    "biases": module.bias.detach().cpu().numpy().tolist()
                }
                
                # Determine activation
                if i + 1 < len(self.model.network):
                    next_module = self.model.network[i + 1]
                    if isinstance(next_module, nn.ReLU):
                        layer_data["activation"] = "ReLU"
                    else:
                        layer_data["activation"] = "Linear"
                else:
                    layer_data["activation"] = "Softmax"  # Output layer
                
                model_data["network_architecture"]["layers"].append(layer_data)
                layer_idx += 1
        
        # Class labels
        for i, label in enumerate(self.label_encoder.classes_):
            model_data["class_labels"][str(i)] = {
                "id": int(label),
                "name": f"Model_{label}",
                "family": "auto_detected"
            }
        
        # Save to file
        with open(output_path, 'w') as f:
            json.dump(model_data, f, indent=2)
        
        print(f"Model exported to SupraFit format: {output_path}")

def main():
    parser = argparse.ArgumentParser(description='Train SupraFit ML model')
    parser.add_argument('--input', required=True, help='Training data JSON file')
    parser.add_argument('--output', default='suprafit_model.json', help='Output model file')
    parser.add_argument('--epochs', type=int, default=1000, help='Training epochs')
    parser.add_argument('--batch-size', type=int, default=64, help='Batch size')
    parser.add_argument('--learning-rate', type=float, default=0.001, help='Learning rate')
    
    args = parser.parse_args()
    
    # Training configuration
    config = {
        'hidden_sizes': [128, 64, 32],
        'dropout_rate': 0.3,
        'learning_rate': args.learning_rate,
        'weight_decay': 1e-4,
        'batch_size': args.batch_size,
        'epochs': args.epochs,
        'early_stopping_patience': 50
    }
    
    # Initialize trainer
    trainer = ModelTrainer(config)
    
    # Load and preprocess data
    X, y = trainer.load_and_preprocess_data(args.input)
    
    # Split data
    X_train, X_test, y_train, y_test = train_test_split(
        X, y, test_size=0.2, random_state=42, stratify=y)
    X_train, X_val, y_train, y_val = train_test_split(
        X_train, y_train, test_size=0.2, random_state=42, stratify=y_train)
    
    print(f"Data split - Train: {len(X_train)}, Val: {len(X_val)}, Test: {len(X_test)}")
    
    # Train model
    history = trainer.train_model(X_train, y_train, X_val, y_val)
    
    # Evaluate model
    results = trainer.evaluate_model(X_test, y_test)
    
    print(f"\nTest Accuracy: {results['accuracy']:.4f}")
    print(f"Best Validation Accuracy: {history['best_val_accuracy']:.2f}%")
    
    # Export model
    trainer.export_to_suprafit_format(args.output)
    
    print(f"\nTraining complete! Model saved to: {args.output}")

if __name__ == "__main__":
    main()
```

## Complete Training Workflow

### 1. Full Pipeline Script

```bash
#!/bin/bash
# training/full_training_pipeline.sh
# Complete SupraFit ML training pipeline

set -e

SUPRAFIT_CLI="../build/release/bin/linux/suprafit_cli"
TRAINING_DIR="training_data"
MODEL_OUTPUT="suprafit_model_v1.json"

echo "🚀 Starting SupraFit ML Training Pipeline"

# Step 1: Generate training data
echo "📊 Generating training data..."
python generate_training_data.py \
    --cli-path "$SUPRAFIT_CLI" \
    --output-dir "$TRAINING_DIR" \
    --models "1,2,3,10,11,20" \
    --samples-per-model 2000 \
    --workers 8

# Step 2: Train neural network
echo "🧠 Training neural network..."
python train_model.py \
    --input "$TRAINING_DIR/consolidated_training_data.json" \
    --output "$MODEL_OUTPUT" \
    --epochs 2000 \
    --batch-size 128 \
    --learning-rate 0.0005

# Step 3: Validate model
echo "✅ Validating trained model..."
python validate_model.py \
    --model "$MODEL_OUTPUT" \
    --test-data "validation_dataset.json" \
    --cli-path "$SUPRAFIT_CLI"

# Step 4: Install model
echo "📦 Installing model for SupraFit..."
mkdir -p ../share/suprafit/ml_models/
cp "$MODEL_OUTPUT" ../share/suprafit/ml_models/default_model.json

echo "🎉 Training pipeline complete!"
echo "Model available at: ../share/suprafit/ml_models/default_model.json"
```

### 2. Model Validation Script

```python
#!/usr/bin/env python3
"""
training/validate_model.py
Comprehensive model validation for SupraFit ML models
"""

import json
import numpy as np
import subprocess
from pathlib import Path
import argparse

def validate_model_accuracy(model_path, test_data_path, cli_path):
    """Test model accuracy against known test cases"""
    
    # Load test data
    with open(test_data_path, 'r') as f:
        test_data = json.load(f)
    
    correct_predictions = 0
    total_predictions = 0
    
    for sample in test_data['training_samples']:
        # Extract features and true label
        true_model_id = sample['label']['correct_model_id']
        
        # Create temporary input file for CLI prediction
        temp_input = create_temp_input_file(sample)
        
        try:
            # Run SupraFit CLI with model prediction
            cmd = [cli_path, '--predict-models', '-i', temp_input, 
                   '--ml-model', model_path]
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=60)
            
            if result.returncode == 0:
                # Parse prediction result
                predicted_model_id = parse_prediction_output(result.stdout)
                
                if predicted_model_id == true_model_id:
                    correct_predictions += 1
                
                total_predictions += 1
            
        except Exception as e:
            print(f"Prediction failed for sample: {e}")
        finally:
            # Clean up temp file
            if Path(temp_input).exists():
                Path(temp_input).unlink()
    
    accuracy = correct_predictions / total_predictions if total_predictions > 0 else 0
    
    return {
        'accuracy': accuracy,
        'correct': correct_predictions,
        'total': total_predictions
    }

def create_temp_input_file(sample):
    """Create temporary input file from test sample"""
    # Implementation depends on how we want to test
    # Could recreate the original configuration or use processed features
    pass

def parse_prediction_output(stdout):
    """Parse CLI output to extract predicted model ID"""
    # Parse SupraFit CLI output format
    pass

if __name__ == "__main__":
    # Model validation main function
    pass
```

This comprehensive training pipeline provides a robust foundation for creating high-quality machine learning models for SupraFit while maintaining complete separation from the C++ runtime environment.