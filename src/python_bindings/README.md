# SupraFit Python Bindings

Modern Python interface for SupraFit using pybind11.

## Installation

### From Source

```bash
cd /path/to/SupraFit/build
cmake .. -DPython_Bindings=ON
make -j4
sudo make install  # Or copy the .so file to your Python site-packages
```

### Quick Test (without installation)

```bash
cd /path/to/SupraFit/build
export PYTHONPATH=$PYTHONPATH:$(pwd)
python3 -c "import suprafit; print(suprafit.__version__)"
```

## Usage

### Basic Example

```python
import suprafit as sf

# Load data from file
data = sf.io.load_data("experiment.txt")
print(f"Loaded {data.DataPoints()} data points")

# Set data range
data.setDataBegin(0)
data.setDataEnd(100)

# Create model
model = sf.models.create_model("nmr_1_1", data)

# Set initial parameters
model.setGlobalParameter(0, 1000.0)  # K (binding constant)

# Fit the model
result = sf.models.fit_model(model)
print(f"Fit success: {result['success']}")
print(f"SSE: {result['sse']:.4f}")
print(f"SEy: {result['sey']:.4f}")

# Export results
sf.io.export_results(model, "results.csv", "csv")
```

### Statistical Analysis

```python
import suprafit as sf

# Load and fit model (as above)
data = sf.io.load_data("experiment.txt")
model = sf.models.create_model("itc_1_1", data)
fit_result = sf.models.fit_model(model)

# Monte Carlo uncertainty analysis
mc_result = sf.statistics.monte_carlo(model, iterations=10000, confidence=0.95)
print(f"Monte Carlo converged: {mc_result['converged']}")
print(f"Statistics: {mc_result['statistics']}")

# Cross-validation
cv_result = sf.statistics.cross_validation(model, cv_type=1, folds=5)
print(f"CV Score: {cv_result['cv_score']:.4f}")
print(f"CV Error: {cv_result['cv_error']:.4f}")

# Confidence intervals
ci_result = sf.statistics.confidence_intervals(model, iterations=10000)
print(f"Confidence intervals: {ci_result['intervals']}")

# Statistical summary
summary = sf.statistics.statistical_summary(model)
print(f"Model: {summary['name']}")
print(f"SSE: {summary['sse']:.4f}")
print(f"Parameters: {summary['parameters']}")
print(f"Global parameters: {summary['global_parameters']}")
```

### Model Comparison

```python
import suprafit as sf

# Load data
data = sf.io.load_data("experiment.txt")

# Create and fit multiple models
model1 = sf.models.create_model("nmr_1_1", data)
sf.models.fit_model(model1)

model2 = sf.models.create_model("nmr_2_1", data)
sf.models.fit_model(model2)

# Compare models
comparison = sf.statistics.compare_models([model1, model2])
print("Model comparison:")
print(f"AIC values: {comparison['aic']}")
for i, stats in enumerate(comparison['models']):
    print(f"Model {i+1}: {stats['name']}, SSE={stats['sse']:.4f}")
```

### Working with Data

```python
import suprafit as sf
import numpy as np

# Load data
data = sf.io.load_data("data.txt")

# Access data tables
indep = data.IndependentModel()
dep = data.DependentModel()

print(f"Data shape: {indep.rowCount()} x {indep.columnCount()}")

# Get data as Python lists
indep_data = indep.toList()
dep_data = dep.toList()

# Convert to numpy (if needed)
import numpy as np
indep_np = np.array(indep_data)
dep_np = np.array(dep_data)

# Save modified data
sf.io.save_data(data, "output.json", "json")
```

## Available Models

Get list of all available models:

```python
import suprafit as sf

models = sf.models.available_models()
print("Available models:", models)
```

Supported model types:
- **NMR Titration**: `nmr_1_1`, `nmr_2_1`, `nmr_1_1_1_2`, `nmr_2_1_1_1`
- **ITC**: `itc_1_1`, `itc_1_2`, `itc_2_1`, `itc_2_2`
- **Fluorescence**: `fl_1_1`, `fl_1_1_1_2`
- **UV-Vis**: `uv_1_1`, `uv_1_1_1_2`

## File Formats

Supported input formats:
```python
formats = sf.io.supported_formats()
print("Input formats:", formats['input'])   # ['txt', 'dat', 'json', 'itc', 'csv']
print("Output formats:", formats['output'])  # ['json', 'txt', 'csv']
```

## API Reference

### Data Module (`sf.data`)

- `DataClass()` - Main data container
  - `Size()` - Number of data points
  - `DataPoints()` - Number of data points
  - `SeriesCount()` - Number of series
  - `IndependentModel()` - Get independent variable table
  - `DependentModel()` - Get dependent variable table
  - `setDataBegin(int)` - Set data range start
  - `setDataEnd(int)` - Set data range end

- `DataTable()` - Data table (matrix)
  - `rowCount()` - Number of rows
  - `columnCount()` - Number of columns
  - `data(row, col)` - Get value at position
  - `setData(row, col, value)` - Set value at position
  - `toList()` - Convert to Python list

### Models Module (`sf.models`)

- `create_model(type, data)` - Create model instance
- `fit_model(model)` - Fit model to data
- `available_models()` - List available model types

### Statistics Module (`sf.statistics`)

- `monte_carlo(model, iterations, confidence)` - Monte Carlo analysis
- `cross_validation(model, cv_type, folds)` - Cross-validation
- `calculate_aic(models)` - AIC comparison
- `compare_models(models)` - Multi-criteria comparison
- `confidence_intervals(model, iterations, lower, upper)` - Percentile-based CI
- `statistical_summary(model)` - Comprehensive summary

### I/O Module (`sf.io`)

- `load_data(filename, format)` - Load data from file
- `save_data(data, filename, format)` - Save data to file
- `load_model(filename)` - Load model from JSON
- `save_model(model, filename, include_statistics)` - Save model to JSON
- `export_results(model, filename, format)` - Export results (CSV, TXT)
- `supported_formats()` - List supported formats

## Requirements

- Python 3.6+
- SupraFit libraries (core, models)
- Qt6 Core and Qml
- pybind11 (automatically downloaded by CMake if not found)

## License

GNU General Public License v3.0

Copyright (C) 2025 Conrad Hübler <Conrad.Huebler@gmx.net>

Python bindings created by Claude Code AI Assistant.
