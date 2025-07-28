# UI - SupraFit Graphical User Interface

## Overview
Complete graphical user interface for SupraFit providing intuitive access to all analytical capabilities. Built with Qt6, offering modern UI components for data visualization, model fitting, and result analysis.

## Core Components

### Main Application
- **main.cpp**: GUI application entry point
- **mainwindow/suprafitgui.cpp/h**: Primary application window and controller
- **mainwindow/mainwindow.cpp/h**: Main window implementation with menu system

### Data Management
- **mainwindow/datawidget.cpp/h**: Data table display and editing
- **mainwindow/projecttree.cpp/h**: Project hierarchy and navigation
- **mainwindow/modeldataholder.cpp/h**: Model-data association management

### Visualization
- **mainwindow/chartwidget.cpp/h**: Main charting and plotting interface
- **widgets/modelchart.cpp/h**: Model-specific visualization components
- **guitools/chartwrapper.cpp/h**: Chart abstraction and utilities

### Model Interface
- **mainwindow/modelwidget.cpp/h**: Model parameter configuration and fitting
- **mainwindow/metamodelwidget.cpp/h**: Meta-model and comparison interfaces
- **widgets/modelelement.cpp/h**: Individual model component representation

## Dialog System

### Data Import/Export
- **dialogs/importdata.cpp/h**: Data file import wizard
- **dialogs/spectraimport.cpp/h**: Spectral data import specialized interface
- **dialogs/thermogram.cpp/h**: Thermogram data handling

### Analysis Configuration
- **dialogs/generatedatadialog.cpp/h**: Data generation parameter setup
- **dialogs/parameterdialog.cpp/h**: Model parameter configuration
- **dialogs/configdialog.cpp/h**: Application settings and preferences

### Results and Analysis
- **dialogs/resultsdialog.cpp/h**: Fitting results presentation
- **dialogs/statisticdialog.cpp/h**: Statistical analysis and validation
- **dialogs/comparedialog.cpp/h**: Model comparison and selection
- **dialogs/regressionanalysisdialog.cpp/h**: Regression analysis tools

### Advanced Tools
- **dialogs/advancedsearch.cpp/h**: Global optimization parameter search
- **dialogs/modaldialog.cpp/h**: Generic modal dialog framework
- **dialogs/genericwidgetdialog.cpp/h**: Reusable widget containers

## Widget Library

### Input Controls
- **widgets/buttons/scientificbox.h**: Scientific notation input
- **widgets/buttons/spinbox.h**: Enhanced numeric input
- **widgets/buttons/hovercheckbox.h**: Interactive checkbox with hover effects

### Data Widgets
- **widgets/DropTable.h**: Drag-and-drop data table interface
- **widgets/parameterwidget.cpp/h**: Parameter input and validation
- **widgets/systemparameterwidget.cpp/h**: System-level parameter configuration

### Specialized Components
- **widgets/spectrawidget.cpp/h**: Spectral data visualization
- **widgets/thermogramwidget.cpp/h**: Thermogram display and analysis
- **widgets/exportsimulationwidget.cpp/h**: Simulation export configuration

### Results Display
- **widgets/results/resultswidget.cpp/h**: Main results presentation
- **widgets/results/mcresultswidget.cpp/h**: Monte Carlo results visualization
- **widgets/results/scatterwidget.cpp/h**: Scatter plot analysis
- **widgets/results/searchresultwidget.cpp/h**: Search results presentation

### Optimization Interface
- **widgets/optimizerwidget.cpp/h**: Optimization algorithm configuration
- **widgets/optionswidget.cpp/h**: General options and settings
- **widgets/preparewidget.cpp/h**: Data preparation and validation

## User Interface Features

### Modern Qt6 Interface
- Responsive layout system with FlowLayout
- High-DPI display support
- Theming and customization options
- Keyboard shortcuts and accessibility

### Data Visualization
- Interactive plotting with zoom and pan
- Multiple chart types (scatter, line, contour)
- Real-time data updates during fitting
- Export capabilities (PNG, SVG, PDF)

### Project Management
- Hierarchical project organization
- Session management and auto-save
- Undo/redo functionality
- Project templates and examples

### Model Integration
- Visual parameter adjustment with sliders
- Real-time model preview
- Parameter correlation visualization
- Confidence interval display

## GUI Tools and Utilities

### Layout Management
- **guitools/flowlayout.cpp/h**: Responsive flow-based layouts
- **guitools/flowlayout.cpp/h**: Custom layout algorithms

### Interaction Tools
- **guitools/mime.h**: Drag-and-drop MIME type handling
- **guitools/waiter.h**: Progress indication and user feedback
- **widgets/messagedock.cpp/h**: Message logging and status display

### Chart Integration
- **guitools/chartwrapper.cpp/h**: Unified charting interface
- **widgets/niceticks.cpp/h**: Automatic axis scaling and formatting

## Current Implementation Status

### ✅ Core Functionality
- Complete data import/export workflows
- Model parameter configuration and fitting
- Results visualization and analysis
- Project management and persistence

### 🔧 Key Integrations
- **CLI Integration**: Seamless data exchange with command-line tools
- **Model System**: Direct access to all analytical models
- **File Formats**: Support for multiple data formats
- **Export Capabilities**: Publication-ready output generation

## Dependencies
- **Qt6**: Widgets, Charts, Qml modules
- **Core libraries**: Complete SupraFit core functionality
- **Visualization**: Qt Charts and custom plotting components

## Usage Patterns

### Typical Workflow
1. Import data via dialogs/importdata
2. Configure model parameters in modelwidget
3. Perform fitting and optimization
4. Analyze results in results widgets
5. Export publication-ready figures

### Advanced Features
- Monte Carlo error analysis
- Global parameter search
- Model comparison and selection
- Batch processing capabilities

---

## Variable Section (Short-term information, regularly updated)

### Recent UI Updates
- Modern Qt6 migration completed
- Enhanced chart visualization capabilities
- Improved responsive layout system
- Better accessibility and keyboard navigation

### Current UI Status
- **Stability**: ✅ All major workflows functional
- **Performance**: ✅ Responsive for typical datasets
- **Usability**: ✅ Intuitive interface design
- **Compatibility**: ✅ Cross-platform deployment working

### Known UI Issues
- None currently identified

### User Experience Priorities
1. Streamlined data import workflows
2. Enhanced real-time visualization during fitting
3. Improved parameter correlation displays
4. Better mobile/tablet compatibility

### Development Guidelines
- Follow Qt6 best practices
- Maintain responsive design principles
- Ensure accessibility compliance
- Provide comprehensive user documentation

### Performance Considerations
- Chart rendering optimized for large datasets
- Asynchronous operations for long-running tasks
- Memory management for visualization components
- Efficient data binding and updates

---

## Instructions Block (Operator-Defined Tasks and Vision)

### Future Tasks
- refactor the whole user interface, details will follow
- if loading simulation files using DataGenerator, give comprehensive information and execute datageneration and open generated project file
- add widget to generate and control simulation input files
- move all thermogramm analysis to core / or finalise the move
- projecttree- logik sollte mit core/**Project Analysis Migration** synchronisiert werden
### Vision
<!-- Add long-term architectural goals here -->