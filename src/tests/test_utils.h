/*
 * Test Utilities for SupraFit CLI Tests
 * Copyright (C) 2025 Conrad Hübler <Conrad.Huebler@gmx.net>
 *
 * Provides standardized CLI binary path resolution and command execution
 * utilities for all CLI-dependent tests. Supports environment variable
 * override for flexible binary location specification.
 * Claude Generated - 2025-09-04
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include <QString>
#include <QStringList>
#include <QCoreApplication>
#include <QFile>
#include <QProcess>
#include <stdexcept>

namespace TestUtils {
    
    /**
     * @brief Find suprafit_cli executable using environment variable or fallback paths
     * 
     * Searches for the suprafit_cli binary in the following order:
     * 1. SUPRAFIT_CLI_PATH environment variable (if set and file exists)
     * 2. Fallback to known relative paths from current working directory
     * 3. Additional build-specific paths (debug/release directories)
     * 
     * @return Absolute or relative path to suprafit_cli executable
     * @throws std::runtime_error if binary not found in any location
     */
    QString findSuprafitCli();
    
    /**
     * @brief Execute CLI command with standardized path resolution and error handling
     * 
     * Executes suprafit_cli with given arguments using the binary found by findSuprafitCli().
     * Provides consistent timeout handling and result parsing for all CLI tests.
     * 
     * @param arguments Command line arguments to pass to suprafit_cli
     * @param timeoutMs Timeout in milliseconds (default: 30000 = 30 seconds)
     * @return QStringList with [exitCode, stdout, stderr] as string representations
     * 
     * Usage example:
     * QStringList result = TestUtils::executeCliCommand({"-i", "config.json"});
     * int exitCode = result[0].toInt();
     * QString stdout = result[1];
     * QString stderr = result[2];
     */
    QStringList executeCliCommand(const QStringList& arguments, int timeoutMs = 30000);
    
    /**
     * @brief Get current CLI binary path (cached after first findSuprafitCli() call)
     * @return Path to suprafit_cli executable
     */
    QString getCliPath();
    
    /**
     * @brief Force re-detection of CLI binary path (for testing different scenarios)
     */
    void resetCliPath();
}