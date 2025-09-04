/*
 * SupraFit CLI - Refactored Main Entry Point
 * Copyright (C) 2018 - 2025 Conrad Hübler <Conrad.Huebler@gmx.net>
 *
 * Modular CLI implementation using command pattern.
 * Separates argument parsing from command execution.
 * Claude Generated - 2025-09-04
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QLoggingCategory>
#include <QtCore/QCommandLineParser>

#include "cli_command_parser.h"
#include "cli_command_dispatcher.h"
#include "suprafit_cli.h"

#include "src/global.h"
#include "src/global_config.h"
#include "src/version.h"

#include <iostream>

#ifndef _WIN32
#if __GNUC__
// Crash handler for debugging
#include <execinfo.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void bt_handler(int sig)
{
    void* array[10];
    size_t size = backtrace(array, 10);

    fprintf(stderr, "SupraFit CLI crashed with signal %d:\n", sig);
    backtrace_symbols_fd(array, size, STDERR_FILENO);
    fprintf(stderr, "Exiting...\n");
    exit(1);
}
#endif
#endif

// Configure logging based on build type
void setupLogging()
{
#ifdef QT_DEBUG
    QLoggingCategory::setFilterRules("*.debug=true");
#else
    QLoggingCategory::setFilterRules("*.debug=false");
#endif
}

// Initialize application metadata
void setupApplication(QCoreApplication& app)
{
    app.setApplicationName("SupraFit CLI");
    app.setApplicationVersion(SUPRAFIT_VERSION);
    app.setOrganizationName("Conrad Hübler");
    app.setOrganizationDomain("suprafit.de");
    
    // Set up application properties for thread management
    app.setProperty("threads", 4); // Default thread count
}

// Enhanced error reporting
void reportError(const QString& operation, const QString& details, int exitCode)
{
    std::cerr << "Error during " << operation.toStdString() 
              << ": " << details.toStdString() << std::endl;
    
    if (exitCode != 0) {
        std::cerr << "Exit code: " << exitCode << std::endl;
    }
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    
    setupApplication(app);
    setupLogging();

#ifndef _WIN32
#if __GNUC__
    // Install crash handler for debugging
    signal(SIGSEGV, bt_handler);
    signal(SIGABRT, bt_handler);
#endif
#endif

    try {
        // Parse command line arguments
        CliCommandParser parser;
        CliCommandParser::ParsedCommand command = parser.parseArguments(app.arguments());
        
        // Handle help and version specially to avoid creating SupraFitCli unnecessarily
        if (command.type == CliCommandParser::CommandType::Help) {
            std::cout << parser.getHelpText().toStdString() << std::endl;
            return 0;
        }
        
        if (command.type == CliCommandParser::CommandType::Version) {
            std::cout << parser.getVersionText().toStdString() << std::endl;
            return 0;
        }
        
        // Validate command before creating CLI instance
        if (!command.isValid) {
            reportError("argument parsing", command.errorMessage, 1);
            std::cerr << "\nUse --help for usage information." << std::endl;
            return 1;
        }
        
        // Create CLI instance for command execution
        SupraFitCli suprafitCli;
        
        // Create command dispatcher
        CliCommandDispatcher dispatcher;
        
        // Connect signals for logging (optional)
        QObject::connect(&dispatcher, &CliCommandDispatcher::commandStarted,
                        [](const QString& description) {
            qDebug() << "Starting command:" << description;
        });
        
        QObject::connect(&dispatcher, &CliCommandDispatcher::commandCompleted,
                        [](int exitCode) {
            if (exitCode == 0) {
                qDebug() << "Command completed successfully";
            } else {
                qDebug() << "Command failed with exit code:" << exitCode;
            }
        });
        
        // Dispatch command
        int exitCode = dispatcher.dispatch(command, suprafitCli);
        
        if (exitCode != 0) {
            reportError("command execution", "See above for details", exitCode);
        }
        
        return exitCode;
        
    } catch (const std::exception& e) {
        reportError("application execution", QString("Unhandled exception: %1").arg(e.what()), -1);
        return -1;
    } catch (...) {
        reportError("application execution", "Unknown exception occurred", -1);
        return -1;
    }
}