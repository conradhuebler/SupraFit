/*
 * Test Utilities for SupraFit CLI Tests - Implementation
 * Copyright (C) 2025 Conrad Hübler <Conrad.Huebler@gmx.net>
 *
 * Provides standardized CLI binary path resolution and command execution
 * utilities for all CLI-dependent tests. Supports environment variable
 * override for flexible binary location specification.
 * Claude Generated - 2025-09-04
 */

#include "test_utils.h"
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QStandardPaths>

namespace TestUtils {
    
// Static variable to cache the CLI path after first discovery
static QString s_cachedCliPath;
static bool s_pathResolved = false;

QString findSuprafitCli()
{
    // Return cached path if already resolved
    if (s_pathResolved && !s_cachedCliPath.isEmpty()) {
        return s_cachedCliPath;
    }
    
    // 1. Check environment variable first (highest priority)
    QString envPath = qEnvironmentVariable("SUPRAFIT_CLI_PATH");
    if (!envPath.isEmpty()) {
        if (QFile::exists(envPath)) {
            qDebug() << "TestUtils: Using CLI path from environment variable:" << envPath;
            s_cachedCliPath = envPath;
            s_pathResolved = true;
            return envPath;
        } else {
            qWarning() << "TestUtils: SUPRAFIT_CLI_PATH environment variable set but file does not exist:" << envPath;
        }
    }
    
    // 2. Fallback to comprehensive path search
    QStringList fallbackPaths = {
        // Current application directory
        QCoreApplication::applicationDirPath() + "/bin/linux/suprafit_cli",
        
        // Relative paths from test execution directory
        "../bin/linux/suprafit_cli",          // Most common case
        "bin/linux/suprafit_cli",             // Direct build output
        "./bin/linux/suprafit_cli",           // Current directory
        
        // Build-specific directories
        "../../release/bin/linux/suprafit_cli", // From tests/ directory to release/
        "../release/bin/linux/suprafit_cli",    // From build/tests/ to build/release/
        "./release/bin/linux/suprafit_cli",     // From project root
        "release/bin/linux/suprafit_cli",       // From project root (no ./)
        
        // Debug build directories
        "../../debug/bin/linux/suprafit_cli",
        "../debug/bin/linux/suprafit_cli", 
        "./debug/bin/linux/suprafit_cli",
        "debug/bin/linux/suprafit_cli",
        
        // Alternative common locations
        "../../bin/linux/suprafit_cli",
        "./suprafit_cli",                     // Direct executable in current dir
        "../suprafit_cli"                     // Direct executable one level up
    };
    
    // Search through all fallback paths
    for (const QString& path : fallbackPaths) {
        if (QFile::exists(path)) {
            qDebug() << "TestUtils: Found CLI executable at:" << path;
            s_cachedCliPath = path;
            s_pathResolved = true;
            return path;
        }
    }
    
    // If we reach here, the binary was not found
    QString errorMsg = QString("suprafit_cli executable not found. Searched paths:\n");
    if (!envPath.isEmpty()) {
        errorMsg += QString("  SUPRAFIT_CLI_PATH: %1 (not found)\n").arg(envPath);
    }
    for (const QString& path : fallbackPaths) {
        errorMsg += QString("  %1\n").arg(path);
    }
    errorMsg += "\nSet SUPRAFIT_CLI_PATH environment variable or ensure binary is built.";
    
    throw std::runtime_error(errorMsg.toStdString());
}

QStringList executeCliCommand(const QStringList& arguments, int timeoutMs)
{
    // Get CLI path (will throw if not found)
    QString cliPath = findSuprafitCli();
    
    // Set up process
    QProcess process;
    process.start(cliPath, arguments);
    
    // Wait for completion
    if (!process.waitForFinished(timeoutMs)) {
        // Process timed out or failed to start
        QStringList result;
        result << QString::number(-1);  // Exit code -1 for timeout/failure
        result << "";                   // Empty stdout
        result << QString("Process timed out after %1ms or failed to start").arg(timeoutMs);
        return result;
    }
    
    // Collect results
    QStringList result;
    result << QString::number(process.exitCode());
    result << QString::fromUtf8(process.readAllStandardOutput());
    result << QString::fromUtf8(process.readAllStandardError());
    
    return result;
}

QString getCliPath()
{
    return findSuprafitCli(); // This will use cached path if available
}

void resetCliPath()
{
    s_cachedCliPath.clear();
    s_pathResolved = false;
    qDebug() << "TestUtils: CLI path cache reset";
}

} // namespace TestUtils