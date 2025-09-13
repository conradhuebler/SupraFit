/*
 * CLI Command Dispatcher for SupraFit
 * Copyright (C) 2025 Conrad Hübler <Conrad.Huebler@gmx.net>
 *
 * Implements command pattern for dispatching CLI operations to appropriate handlers.
 * Separates command parsing from execution logic.
 * Claude Generated - 2025-09-04
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QSharedPointer>

#include "cli_command_parser.h"

// Forward declarations
class SupraFitCli;

/**
 * @brief Abstract base class for CLI commands
 * 
 * Implements command pattern for modular CLI operations.
 * Each command type has its own concrete implementation.
 */
class CliCommand
{
public:
    virtual ~CliCommand() = default;
    
    /**
     * @brief Execute the command
     * @param suprafitCli Reference to CLI instance for operations
     * @return Exit code (0 for success, non-zero for error)
     */
    virtual int execute(SupraFitCli& suprafitCli) = 0;
    
    /**
     * @brief Get command description for logging/debugging
     * @return Human-readable command description
     */
    virtual QString description() const = 0;
};

/**
 * @brief Command dispatcher that routes parsed commands to appropriate handlers
 * 
 * Implements the command pattern to separate argument parsing from command execution.
 * Maintains modular architecture and enables easy testing of individual commands.
 */
class CliCommandDispatcher : public QObject
{
    Q_OBJECT

public:
    explicit CliCommandDispatcher(QObject* parent = nullptr);
    virtual ~CliCommandDispatcher() = default;

    /**
     * @brief Dispatch parsed command to appropriate handler
     * @param command Parsed command from CliCommandParser
     * @param suprafitCli CLI instance to execute operations on
     * @return Exit code (0 for success, non-zero for error)
     */
    int dispatch(const CliCommandParser::ParsedCommand& command, SupraFitCli& suprafitCli);

private:
    /**
     * @brief Create appropriate command object for command type
     * @param command Parsed command
     * @return Shared pointer to command object
     */
    QSharedPointer<CliCommand> createCommand(const CliCommandParser::ParsedCommand& command);

signals:
    /**
     * @brief Emitted when command execution starts
     * @param description Command description
     */
    void commandStarted(const QString& description);
    
    /**
     * @brief Emitted when command execution completes
     * @param exitCode Command exit code
     */
    void commandCompleted(int exitCode);
};

// Concrete command implementations

/**
 * @brief Help command implementation
 */
class HelpCommand : public CliCommand
{
public:
    explicit HelpCommand(const QString& helpText) : m_helpText(helpText) {}
    int execute(SupraFitCli& suprafitCli) override;
    QString description() const override { return "Display help information"; }

private:
    QString m_helpText;
};

/**
 * @brief Version command implementation
 */
class VersionCommand : public CliCommand
{
public:
    explicit VersionCommand(const QString& versionText) : m_versionText(versionText) {}
    int execute(SupraFitCli& suprafitCli) override;
    QString description() const override { return "Display version information"; }

private:
    QString m_versionText;
};

/**
 * @brief File structure listing command
 */
class ListCommand : public CliCommand
{
public:
    explicit ListCommand(const QString& filename) : m_filename(filename) {}
    int execute(SupraFitCli& suprafitCli) override;
    QString description() const override { return QString("List structure of file: %1").arg(m_filename); }

private:
    QString m_filename;
};

/**
 * @brief Data generation command
 */
class GenerateCommand : public CliCommand
{
public:
    explicit GenerateCommand(const QString& inputFile, const QString& outputFile, int threads)
        : m_inputFile(inputFile), m_outputFile(outputFile), m_threads(threads) {}
    int execute(SupraFitCli& suprafitCli) override;
    QString description() const override { return QString("Generate data from config: %1").arg(m_inputFile); }

private:
    QString m_inputFile;
    QString m_outputFile;
    int m_threads;
};

/**
 * @brief Parameter extraction command
 */
class ExtractCommand : public CliCommand
{
public:
    explicit ExtractCommand(const QString& filename, const QString& modelIndex = QString())
        : m_filename(filename), m_modelIndex(modelIndex) {}
    int execute(SupraFitCli& suprafitCli) override;
    QString description() const override { 
        if (m_modelIndex.isEmpty()) {
            return QString("Extract all parameters from: %1").arg(m_filename);
        } else {
            return QString("Extract model %1 parameters from: %2").arg(m_modelIndex, m_filename);
        }
    }

private:
    QString m_filename;
    QString m_modelIndex;
};

/**
 * @brief Post-processing analysis display command
 */
class ShowPostProcessingCommand : public CliCommand
{
public:
    explicit ShowPostProcessingCommand(const QString& filename) : m_filename(filename) {}
    int execute(SupraFitCli& suprafitCli) override;
    QString description() const override { return QString("Show post-processing analysis: %1").arg(m_filename); }

private:
    QString m_filename;
};