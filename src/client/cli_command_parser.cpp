/*
 * CLI Command Line Parser implementation
 * Copyright (C) 2025 Conrad Hübler <Conrad.Huebler@gmx.net>
 *
 * Claude Generated - 2025-09-04
 */

#include "cli_command_parser.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QFileInfo>

CliCommandParser::CliCommandParser(QObject* parent)
    : QObject(parent)
{
    setupOptions();
}

void CliCommandParser::setupOptions()
{
    // This method is kept for compatibility but not used anymore
}

void CliCommandParser::setupOptionsOn(QCommandLineParser& parser)
{
    // Help and version options
    parser.addHelpOption();
    parser.addVersionOption();
    
    // Input/Output options
    parser.addOption({{"i", "input"}, "Input configuration file", "file"});
    parser.addOption({{"o", "output"}, "Output file name", "file"});
    
    // Analysis options
    parser.addOption({{"l", "list"}, "List file structure", "file"});
    parser.addOption({{"x", "extract-parameters"}, "Extract model parameters", "file"});
    parser.addOption({"extract-model", "Extract specific model parameters", "model_index"});
    parser.addOption({"show-post-processing", "Show detailed post-processing statistics", "file"});
    
    // Processing options
    parser.addOption({{"n", "nproc"}, "Number of threads", "threads", "4"});
    
    // Positional argument for direct file analysis
    parser.addPositionalArgument("file", "File to analyze (direct mode)", "[file]");
}

CliCommandParser::ParsedCommand CliCommandParser::parseArguments(const QStringList& arguments)
{
    ParsedCommand command;
    
    // Create fresh parser for each parsing operation
    QCommandLineParser parser;
    parser.setApplicationDescription("SupraFit CLI - Supramolecular Chemistry Analysis and Data Generation");
    
    // Setup options on local parser
    setupOptionsOn(parser);
    
    // Parse arguments
    if (!parser.parse(arguments)) {
        command.errorMessage = parser.errorText();
        return command;
    }
    
    // Extract values
    command.inputFile = parser.value("input");
    command.outputFile = parser.value("output");
    command.threads = parser.value("nproc").toInt();
    command.extractModel = parser.value("extract-model");
    command.showPostProcessing = parser.isSet("show-post-processing");
    
    // Handle positional arguments
    QStringList positionalArgs = parser.positionalArguments();
    if (!positionalArgs.isEmpty() && command.inputFile.isEmpty()) {
        command.inputFile = positionalArgs.first();
    }
    
    // Handle list option with file parameter
    if (parser.isSet("list")) {
        QString listFile = parser.value("list");
        if (!listFile.isEmpty()) {
            command.inputFile = listFile;
        }
    }
    
    // Handle extract-parameters option
    if (parser.isSet("extract-parameters")) {
        QString extractFile = parser.value("extract-parameters");
        if (!extractFile.isEmpty()) {
            command.inputFile = extractFile;
        }
    }
    
    // Handle show-post-processing option
    if (parser.isSet("show-post-processing")) {
        QString ppFile = parser.value("show-post-processing");
        if (!ppFile.isEmpty()) {
            command.inputFile = ppFile;
        }
    }
    
    // Determine command type
    command.type = determineCommandType(command, parser);
    
    // Validate command
    command.isValid = validateCommand(command);
    
    return command;
}

CliCommandParser::CommandType CliCommandParser::determineCommandType(const ParsedCommand& command, const QCommandLineParser& parser) const
{
    if (parser.isSet("help")) {
        return CommandType::Help;
    }
    
    if (parser.isSet("version")) {
        return CommandType::Version;
    }
    
    if (parser.isSet("list")) {
        return CommandType::List;
    }
    
    if (parser.isSet("extract-parameters")) {
        return CommandType::Extract;
    }
    
    if (parser.isSet("show-post-processing")) {
        return CommandType::ShowPostProcessing;
    }
    
    if (!command.inputFile.isEmpty()) {
        return CommandType::Generate;
    }
    
    return CommandType::Invalid;
}

bool CliCommandParser::validateCommand(ParsedCommand& command) const
{
    switch (command.type) {
        case CommandType::Help:
        case CommandType::Version:
            return true;
            
        case CommandType::List:
        case CommandType::Extract:
        case CommandType::ShowPostProcessing:
        case CommandType::Generate:
            if (command.inputFile.isEmpty()) {
                command.errorMessage = "Input file is required for this operation";
                return false;
            }
            
            // Check if input file exists
            if (!QFileInfo::exists(command.inputFile)) {
                command.errorMessage = QString("Input file does not exist: %1").arg(command.inputFile);
                return false;
            }
            
            break;
            
        case CommandType::Invalid:
            command.errorMessage = "No valid command specified. Use --help for usage information.";
            return false;
    }
    
    // Validate thread count
    if (command.threads < 1) {
        command.errorMessage = "Thread count must be at least 1";
        return false;
    }
    
    if (command.threads > 64) {
        command.errorMessage = "Thread count should not exceed 64";
        return false;
    }
    
    // Validate extract model index if specified
    if (!command.extractModel.isEmpty()) {
        bool ok;
        int modelIndex = command.extractModel.toInt(&ok);
        if (!ok || modelIndex < 0) {
            command.errorMessage = "Extract model index must be a non-negative integer";
            return false;
        }
    }
    
    return true;
}

QString CliCommandParser::getHelpText() const
{
    return formatUsageText() + "\n\n" + 
           "Examples:\n" +
           "  suprafit_cli -i config.json                    # Generate data\n" +
           "  suprafit_cli -i config.json -o results.json   # Generate with output\n" +
           "  suprafit_cli -l model.suprafit                 # List file structure\n" +
           "  suprafit_cli -x model.suprafit                 # Extract parameters\n" +
           "  suprafit_cli --extract-model 2 model.suprafit # Extract specific model\n" +
           "  suprafit_cli --show-post-processing model.json # Show statistics\n" +
           "  suprafit_cli -n 8 -i config.json              # Use 8 threads\n" +
           "  suprafit_cli config.json                       # Direct file analysis\n";
}

QString CliCommandParser::getVersionText() const
{
    return QString("SupraFit CLI v%1\n"
                  "Supramolecular Chemistry Analysis Framework\n"
                  "Copyright (C) 2019-2025 Conrad Hübler\n"
                  "Built with Qt %2")
                  .arg(QCoreApplication::applicationVersion())
                  .arg(QT_VERSION_STR);
}

QString CliCommandParser::formatUsageText() const
{
    return "Usage: suprafit_cli [options] [file]\n\n"
           "Options:\n"
           "  -h, --help                     Display this help and exit\n"
           "  -v, --version                  Display version information and exit\n"
           "  -i, --input <file>             Input configuration file\n"
           "  -o, --output <file>            Output file name\n"
           "  -l, --list <file>              List file structure and contents\n"
           "  -x, --extract-parameters <file> Extract fitted model parameters\n"
           "      --extract-model <index>    Extract parameters from specific model\n"
           "      --show-post-processing <file> Show detailed statistical analysis\n"
           "  -n, --nproc <threads>          Number of processing threads (default: 4)\n\n"
           "Arguments:\n"
           "  file                           File to analyze (when no -i option specified)";
}

#include "cli_command_parser.moc"