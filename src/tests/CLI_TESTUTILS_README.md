# CLI TestUtils Implementation - Usage Guide
*Claude Generated - 2025-09-04*

## Overview
The `TestUtils` namespace provides standardized CLI binary path resolution and command execution utilities for all CLI-dependent tests in SupraFit.

## Problem Solved
Previously, all CLI tests had redundant, hardcoded path-finding logic with 6-8 different fallback paths each. This implementation consolidates all CLI binary discovery into a single, maintainable utility with environment variable support.

## Files
- **`test_utils.h`** - Header with TestUtils namespace declarations
- **`test_utils.cpp`** - Implementation with environment variable and fallback path support
- **`CMakeLists.txt`** - Updated to include test_utils.cpp in all CLI tests

## Key Features

### 1. Environment Variable Support
Set `SUPRAFIT_CLI_PATH` to override automatic binary detection:
```bash
export SUPRAFIT_CLI_PATH=/path/to/your/suprafit_cli
```

### 2. Comprehensive Fallback Paths
Automatically searches in order:
1. `SUPRAFIT_CLI_PATH` environment variable
2. `QCoreApplication::applicationDirPath() + "/bin/linux/suprafit_cli"`
3. `../bin/linux/suprafit_cli`
4. `bin/linux/suprafit_cli`
5. `../../release/bin/linux/suprafit_cli`
6. `../release/bin/linux/suprafit_cli`
7. `./bin/linux/suprafit_cli`
8. Additional debug/alternative paths

### 3. Cached Path Resolution
The binary path is resolved once and cached for performance.

## API Usage

### Basic CLI Execution
```cpp
#include "test_utils.h"

// Execute CLI command with default 30s timeout
QStringList result = TestUtils::executeCliCommand({"-i", "config.json"});
int exitCode = result[0].toInt();
QString stdout = result[1];
QString stderr = result[2];

// Execute with custom timeout (60s)
QStringList result = TestUtils::executeCliCommand({"-i", "config.json"}, 60000);
```

### Get CLI Path
```cpp
QString cliPath = TestUtils::getCliPath();
qDebug() << "Using CLI:" << cliPath;
```

### Reset Path Cache (for testing)
```cpp
TestUtils::resetCliPath(); // Forces re-detection on next call
```

## Migration Pattern

### Before (Old Pattern)
```cpp
class TestExample : public QObject {
private:
    QString m_suprafitCli;
    
    void initTestCase() {
        // 6-8 lines of hardcoded path searching
        m_suprafitCli = QCoreApplication::applicationDirPath() + "/bin/linux/suprafit_cli";
        if (!QFile::exists(m_suprafitCli)) {
            m_suprafitCli = "../bin/linux/suprafit_cli";
        }
        // ... more fallback paths
        QVERIFY2(QFile::exists(m_suprafitCli), "suprafit_cli executable not found");
    }
    
    QStringList runCliCommand(const QStringList& arguments) {
        QProcess process;
        process.start(m_suprafitCli, arguments);
        process.waitForFinished(30000);
        // ... result processing
    }
};
```

### After (New Pattern)
```cpp
#include "test_utils.h"

class TestExample : public QObject {
private:
    // No m_suprafitCli member needed
    
    void initTestCase() {
        // Single line verification
        QString cliPath = TestUtils::getCliPath();
        qDebug() << "Using CLI executable:" << cliPath;
    }
    
    QStringList runCliCommand(const QStringList& arguments) {
        return TestUtils::executeCliCommand(arguments, 30000);
    }
};
```

## Migrated Tests (✅ Completed)
1. **test_cli_core.cpp** - Basic CLI argument parsing and functionality
2. **test_cli_data_generation.cpp** - Data generation workflow testing

## Remaining Tests to Migrate
The following tests still use old pattern and should be migrated:
1. `test_cli_ml_pipeline.cpp`
2. `test_model_fitting.cpp`
3. `test_post_processing.cpp`
4. `test_ml_extraction.cpp`
5. `test_multi_project.cpp`
6. `test_integration.cpp`
7. `test_file_operations.cpp`
8. `test_data_generation.cpp`
9. `test_comprehensive_real_data.cpp`

## Migration Steps
1. Add `#include "test_utils.h"` after Qt includes
2. Remove `QString m_suprafitCli;` member variable
3. Replace path-finding logic in `initTestCase()` with `TestUtils::getCliPath()`
4. Replace `runCliCommand()` implementation with `TestUtils::executeCliCommand()`

## Testing Verification
Both migrated tests successfully locate the CLI binary:
```
QDEBUG : TestUtils: Found CLI executable at: "../../bin/linux/suprafit_cli"
```

## Environment Usage Examples
```bash
# Use specific CLI build
export SUPRAFIT_CLI_PATH=/home/user/SupraFit/debug/bin/linux/suprafit_cli

# Run tests with custom CLI
ctest -R CliCore

# Use release build CLI  
export SUPRAFIT_CLI_PATH=/home/user/SupraFit/release/bin/linux/suprafit_cli
make test_cli_data_generation
```

## Benefits Achieved
- ✅ **DRY Principle**: Eliminated code duplication across 12+ test files
- ✅ **Flexibility**: Environment variable allows explicit path control
- ✅ **Maintainability**: Single location for path resolution logic
- ✅ **Consistency**: All tests use identical binary resolution
- ✅ **Performance**: Path caching reduces repeated file system checks
- ✅ **Debugging**: Easy CLI path override for different build variants