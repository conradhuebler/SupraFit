/*
 * CLI Command Line Parser for SupraFit
 * Copyright (C) 2025 Conrad Hübler <Conrad.Huebler@gmx.net>
 *
 * Modular command-line argument parsing and validation.
 * Implements command pattern for CLI operations.
 * Claude Generated - 2025-09-04
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include <QtCore/QObject>
#include <QtCore/QCommandLineParser>
#include <QtCore/QCommandLineOption>
#include <QtCore/QString>
#include <QtCore/QStringList>

/**
 * @brief Command line parser for SupraFit CLI
 * 
 * Provides structured parsing and validation of command-line arguments.
 * Separates argument parsing from command execution logic.
 * Claude Generated - supports all current CLI options with enhanced validation.
 */
class CliCommandParser : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Command types supported by the CLI
     */
    enum class CommandType {
        Help,              // --help, -h
        Version,           // --version, -v  
        List,              // --list, -l
        Generate,          // -i input.json (data generation)
        Extract,           // -x, --extract-parameters
        ShowPostProcessing, // --show-post-processing
        Invalid
    };

    /**
     * @brief Parsed command structure
     */
    struct ParsedCommand {
        CommandType type = CommandType::Invalid;
        QString inputFile;
        QString outputFile;
        int threads = 4;
        QString extractModel;
        bool showPostProcessing = false;
        bool isValid = false;
        QString errorMessage;
    };

    explicit CliCommandParser(QObject* parent = nullptr);
    virtual ~CliCommandParser() = default;

    /**
     * @brief Parse command line arguments
     * @param arguments Command line arguments
     * @return Parsed command structure
     */
    ParsedCommand parseArguments(const QStringList& arguments);

    /**
     * @brief Get help text for CLI usage
     * @return Formatted help text
     */
    QString getHelpText() const;

    /**
     * @brief Get version information
     * @return Version string
     */
    QString getVersionText() const;

private:
    void setupOptions();
    void setupOptionsOn(QCommandLineParser& parser);
    CommandType determineCommandType(const ParsedCommand& command, const QCommandLineParser& parser) const;
    bool validateCommand(ParsedCommand& command) const;
    QString formatUsageText() const;
};