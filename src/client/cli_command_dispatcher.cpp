/*
 * CLI Command Dispatcher implementation
 * Copyright (C) 2025 Conrad Hübler <Conrad.Huebler@gmx.net>
 *
 * Claude Generated - 2025-09-04
 */

#include "cli_command_dispatcher.h"
#include "suprafit_cli.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>

#include <iostream>

CliCommandDispatcher::CliCommandDispatcher(QObject* parent)
    : QObject(parent)
{
}

int CliCommandDispatcher::dispatch(const CliCommandParser::ParsedCommand& command, SupraFitCli& suprafitCli)
{
    if (!command.isValid) {
        std::cerr << "Error: " << command.errorMessage.toStdString() << std::endl;
        return 1;
    }

    QSharedPointer<CliCommand> cliCommand = createCommand(command);
    if (!cliCommand) {
        std::cerr << "Error: Unknown command type" << std::endl;
        return 1;
    }

    emit commandStarted(cliCommand->description());

    int exitCode = cliCommand->execute(suprafitCli);

    emit commandCompleted(exitCode);

    return exitCode;
}

QSharedPointer<CliCommand> CliCommandDispatcher::createCommand(const CliCommandParser::ParsedCommand& command)
{
    switch (command.type) {
        case CliCommandParser::CommandType::Help: {
            // We need to get help text from the parser - for now use basic help
            QString helpText = "SupraFit CLI - Use --help for detailed usage information";
            return QSharedPointer<HelpCommand>::create(helpText);
        }

        case CliCommandParser::CommandType::Version: {
            QString versionText = QString("SupraFit CLI v%1").arg(QCoreApplication::applicationVersion());
            return QSharedPointer<VersionCommand>::create(versionText);
        }

        case CliCommandParser::CommandType::List:
            return QSharedPointer<ListCommand>::create(command.inputFile);

        case CliCommandParser::CommandType::Generate:
            return QSharedPointer<GenerateCommand>::create(command.inputFile, command.outputFile, command.threads);

        case CliCommandParser::CommandType::Extract:
            return QSharedPointer<ExtractCommand>::create(command.inputFile, command.extractModel);

        case CliCommandParser::CommandType::ShowPostProcessing:
            return QSharedPointer<ShowPostProcessingCommand>::create(command.inputFile);

        case CliCommandParser::CommandType::Invalid:
        default:
            return nullptr;
    }
}

// Concrete command implementations

int HelpCommand::execute(SupraFitCli& suprafitCli)
{
    Q_UNUSED(suprafitCli)
    
    std::cout << m_helpText.toStdString() << std::endl;
    return 0;
}

int VersionCommand::execute(SupraFitCli& suprafitCli)
{
    Q_UNUSED(suprafitCli)
    
    std::cout << m_versionText.toStdString() << std::endl;
    return 0;
}

int ListCommand::execute(SupraFitCli& suprafitCli)
{
    try {
        suprafitCli.setInFile(m_filename);
        if (!suprafitCli.LoadFile()) {
            std::cerr << "Error: Failed to load file: " << m_filename.toStdString() << std::endl;
            return 1;
        }
        
        suprafitCli.PrintFileStructure();
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error analyzing file: " << e.what() << std::endl;
        return 1;
    }
}

int GenerateCommand::execute(SupraFitCli& suprafitCli)
{
    try {
        suprafitCli.setInFile(m_inputFile);
        
        if (!m_outputFile.isEmpty()) {
            suprafitCli.setOutFile(m_outputFile);
        }
        
        // Set thread count in application properties for consistency
        QCoreApplication::instance()->setProperty("threads", m_threads);
        
        if (!suprafitCli.LoadFile()) {
            std::cerr << "Error: Failed to load configuration file: " << m_inputFile.toStdString() << std::endl;
            return 1;
        }
        
        suprafitCli.Work();
        
        if (!suprafitCli.SaveFile()) {
            std::cerr << "Error: Failed to save output file" << std::endl;
            return 1;
        }
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error generating data: " << e.what() << std::endl;
        return 1;
    }
}

int ExtractCommand::execute(SupraFitCli& suprafitCli)
{
    try {
        bool success = suprafitCli.ExtractModelParameters(m_modelIndex);
        
        if (!success) {
            std::cerr << "Error: Failed to extract parameters from: " << m_filename.toStdString() << std::endl;
            return 1;
        }
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error extracting parameters: " << e.what() << std::endl;
        return 1;
    }
}

int ShowPostProcessingCommand::execute(SupraFitCli& suprafitCli)
{
    try {
        suprafitCli.setInFile(m_filename);
        suprafitCli.setShowPostProcessingDetails(true);
        
        if (!suprafitCli.LoadFile()) {
            std::cerr << "Error: Failed to load file: " << m_filename.toStdString() << std::endl;
            return 1;
        }
        
        suprafitCli.AnalyzeFile();
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error analyzing post-processing data: " << e.what() << std::endl;
        return 1;
    }
}

#include "cli_command_dispatcher.moc"