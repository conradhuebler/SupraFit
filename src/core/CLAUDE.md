# Core - Fundamental SupraFit Components

## Overview
Core functionality providing the foundation for SupraFit applications. Contains data structures, file handling, mathematical operations, and the complete model system for supramolecular chemistry analysis.

## Core Components

### Data Structures
- **models/dataclass.cpp/h**: Main project container with metadata and system parameters
- **models/datatable.cpp/h**: Core data structure using Eigen::MatrixXd for numerical operations
- **models/AbstractModel.cpp/h**: Base class for all analytical models

### File Management  
- **filehandler.cpp/h**: File I/O operations with range selection capabilities (Claude Generated)
- **jsonhandler.cpp/h**: JSON serialization and deserialization for project files
- **thermogramhandler.cpp/h**: Specialized thermogram data processing
- **spectrahandler.cpp/h**: Spectral data import and processing

### Mathematical Operations
- **libmath.cpp/h**: Mathematical utilities and algorithms
- **minimizer.cpp/h**: Optimization algorithms for parameter fitting
- **equil.cpp/h**: Equilibrium calculations for supramolecular systems
- **concentrationalpolynomial.cpp/h**: Concentration-based polynomial calculations

### Analysis Tools
- **analyse.cpp/h**: **Statistical analysis and data processing with JSON API (Claude Generated)**
  - **JSON-based statistical methods**: `CalculateAICMetrics()`, `CalculateMCMetrics()`, `CalculateCVMetrics()`, `CalculateReductionMetrics()`
  - **ML feature extraction**: `ExtractModelMLFeatures()` for standardized training data
  - **Percentile-based confidence intervals**: Monte Carlo analysis with 2.5%/97.5% quantiles
  - **Legacy string-based functions**: `CompareAIC()`, `CompareMC()`, `CompareCV()`, `AnalyseReductionAnalysis()` for backward compatibility
  - **Human-readable formatting**: `FormatStatisticsString()` converts JSON results to HTML tables
- **pythonbridge.cpp/h**: Python integration for extended functionality
- **toolset.cpp/h**: General utility functions and statistical calculations

## Model System

### Titration Models
Comprehensive support for analytical chemistry techniques:

#### NMR Titrations
- **nmr_1_1_Model**: 1:1 binding NMR titrations
- **nmr_1_1_1_2_Model**: 1:1 and 1:2 competitive binding
- **nmr_2_1_1_1_Model**: 2:1 binding with multiple series
- **nmr_any_Model**: General n:m binding models

#### ITC (Isothermal Titration Calorimetry)
- **itc_1_1_Model**: 1:1 binding enthalpy measurements
- **itc_1_2_Model**, **itc_2_1_Model**, **itc_2_2_Model**: Complex binding stoichiometries
- **itc_any_Model**: General binding models
- **itc_n_1_1_Model**, **itc_n_1_2_Model**: Multi-site binding

#### Fluorescence Spectroscopy
- **fl_1_1_Model**: 1:1 binding fluorescence changes
- **fl_1_1_1_2_Model**: Competitive binding with fluorescence
- **fl_2_1_1_1_Model**: Complex binding patterns

#### UV-Vis Spectroscopy
- **uv_vis_1_1_Model**: 1:1 binding UV-Vis changes
- **uv_vis_2_1_1_1_1_2_Model**: Complex multi-equilibrium systems

### Kinetics Models
- **monomolecularmodel**: First-order kinetics
- **bimolecularmodel**: Second-order kinetics  
- **mm_model**: Michaelis-Menten enzyme kinetics
- **flexmolecularmodel**: Flexible kinetic models

### Thermodynamics Models
- **arrhenius**: Arrhenius equation for temperature dependence
- **eyring**: Eyring equation for reaction kinetics
- **bet**: BET isotherm for surface adsorption

### Scripting Integration
- **scriptmodel.cpp/h**: Base class for scripted models
- **chaiinterpreter.cpp/h**: ChaiScript integration
- **pymodelinterpreter.cpp/h**: Python model interpreter
- **exprtkinterpreter.cpp/h**: ExprTk mathematical expression parser

## Key Features

### Enhanced File Handling (Claude Generated)
Precise data range extraction from source files:

```cpp
// Extract specific data ranges
QJsonObject getDataRange(int startRow, int endRow, int startCol, int endCol);

// Range parameters for flexible loading
int m_start_row, m_end_row, m_start_col, m_end_col;
```

### DataClass Capabilities
- Project metadata management
- Independent/dependent data table handling
- Model association and parameter storage
- JSON serialization for project persistence

### DataTable Features  
- Eigen::MatrixXd integration for high-performance numerical operations
- Data validation and consistency checking
- Export/import functionality
- Statistical operations and noise generation

## Model Integration

### Model Creation Pattern
```cpp
// Standard model creation via UI approach
QSharedPointer<AbstractModel> model = AddModel(modelId, dataClass);

// Model calculation and result extraction
model->Calculate();
DataTable* results = model->ModelTable();
```

### Parameter Management
```cpp
// Global parameters (stability constants)
model->setGlobalParameter(value, index);

// Local parameters (chemical shifts, extinction coefficients)
model->setLocalParameter(value, seriesIndex, parameterIndex);
```

## Current Implementation Status

### ✅ Enhanced Features
- **File Range Loading**: Precise row/column selection for modular data generation
- **Model Integration**: Direct access to titration models for data generation
- **Memory Safety**: Improved pointer management and validation
- **JSON Compatibility**: Enhanced serialization for complex data structures

### 🔧 Core Functionality
- Complete model library for supramolecular chemistry
- High-performance numerical operations via Eigen
- Flexible scripting integration (ChaiScript, Python, ExprTk)
- Comprehensive file format support

## Dependencies
- **Qt6**: Core, Qml modules
- **Eigen**: Matrix operations (via libpeakpick)
- **ChaiScript**: Scripting support
- **Python**: Optional interpreter integration
- **ExprTk**: Mathematical expression parsing

## Usage Patterns

### Data Loading with Range Selection
```cpp
FileHandler handler(filename);
handler.LoadFile();
QJsonObject data = handler.getDataRange(startRow, endRow, startCol, endCol);
```

### Model-Based Calculations
```cpp
DataClass* data = new DataClass();
data->setIndependentTable(independentData);
QSharedPointer<AbstractModel> model = AddModel(modelId, data);
model->Calculate();
```

---

## Variable Section (Short-term information, regularly updated)

### Recent Changes

### Current Status
- JSON serialization stable and reliable
- No memory leaks or crashes identified

### Known Issues
- None currently identified

### Performance Notes
- Eigen operations provide excellent numerical performance
- File I/O optimized for large datasets
- Model calculations scale efficiently with data size

### Testing Status
- DataTable tests: 18/25 passing (some edge cases)
- DataClass tests: Most functionality working (one crash test)
- File handling: ✅ All range selection tests passing

---

## Instructions Block (Operator-Defined Tasks and Vision)

### Future Tasks (Restructured 2025-01-28)

#### **🔥 HIGH PRIORITY** - Implement immediately:
1. **Statistical Analysis JSON API** (Task #1)
   - NOT: Extract data from strings
   - INSTEAD: Implement JSON-based statistical methods (`getStatisticsJSON()`)
   - String output should be based on JSON (`formatStatisticsString(json)`)
   - Implement: AIC, SSE, R², parameter uncertainty, prediction variance
   - Goal: Unified statistical analysis API for all client applications

2. **Pre-compiled Headers Implementation** (Task #2)
   - Add PCH support for faster compilation
   - `#include "pch.h"` with Qt6/Eigen/fmt headers
   - Significantly reduced build times during development

3. **ML Pipeline Performance Optimization** (Task #3)
   - Accelerate data generation for productive use
   - Optimize DataGenerator algorithms
   - Focus on speed, not parallelization

#### **⚡ MEDIUM PRIORITY**:
4. **Project Analysis Migration**: Move analysis from src/client/suprafit_cli.cpp to core-libs
5. **Scripted Models**: Improve functionality and integration

#### **📋 LOW PRIORITY** - Long-term:
- Refactor bc50 code, no more inlining
- Refactor optimizer logic, contains much old duplicate code
- Implement BFGS optimization as alternative to concentration solver (DOI: 10.1039/d4sc03354j, Citation: Daniil O. Soloviev and Christopher A. Hunter; Chem. Sci., 2024,15, 15299-15310)
### Vision
- **LLM Support for SupraFit**: Based on parsed projects, evaluation should be possible in natural language using local LLMs (if connected). Context information and specialist publication knowledge should be provided or passed on as needed. 