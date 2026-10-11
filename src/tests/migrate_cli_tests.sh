#!/bin/bash
# Batch migration script for CLI tests to use TestUtils
# Claude Generated - 2025-09-04

CLI_TESTS=(
    "test_cli_ml_pipeline.cpp"
    "test_model_fitting.cpp" 
    "test_post_processing.cpp"
    "test_ml_extraction.cpp"
    "test_multi_project.cpp"
    "test_integration.cpp"
    "test_file_operations.cpp"
    "test_data_generation.cpp"
    "test_comprehensive_real_data.cpp"
)

for test_file in "${CLI_TESTS[@]}"; do
    echo "Migrating $test_file..."
    
    # Add test_utils.h include after the last QtCore include
    sed -i '/^#include <QtCore\/.*>/a #include "test_utils.h"' $test_file
    
    # Remove m_suprafitCli member variable declarations
    sed -i '/QString m_suprafitCli;/d' $test_file
    
    # Remove the complex CLI path search logic from initTestCase functions
    # This is a multi-line replacement, so we'll use a more complex approach
    
    # Replace the CLI path finding logic with TestUtils call
    perl -i -0pe 's/\/\/ Find suprafit_cli executable.*?qDebug\(\) << "Using CLI executable:" << m_suprafitCli;/\/\/ Verify CLI executable is available via TestUtils\n    QString cliPath = TestUtils::getCliPath();\n    qDebug() << "Using CLI executable:" << cliPath;/gs' $test_file
    
    # Replace runCliCommand implementations
    perl -i -0pe 's/QStringList (\w+::runCliCommand\(const QStringList& arguments[^}]*\)\s*\{)\s*QProcess process;\s*process\.start\(m_suprafitCli, arguments\);\s*process\.waitForFinished\(\d+\);[^}]*return result;\s*\}/QStringList $1\n{\n    return TestUtils::executeCliCommand(arguments, 60000);\n}/gs' $test_file
    
    echo "Migrated $test_file"
done

echo "All CLI tests migrated to use TestUtils!"