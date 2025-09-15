/*
 * ML Pipeline CLI for SupraFit with Neural Network Tutorials
 * Copyright (C) 2019 - 2025 Conrad Hübler <Conrad.Huebler@gmx.net>
 */

#include <QtCore/QCoreApplication>
#include <QtCore/QCommandLineParser>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

#include "src/ml/tutorial/examples/xor_tutorial.h"
#include "src/ml/tutorial/examples/nmr_model_selection_tutorial.h"
#include "src/ml/tutorial/examples/training_tutorial.h"

void printUsage(const QString& programName)
{
    qDebug() << "ML Pipeline CLI for SupraFit with Neural Network Tutorials";
    qDebug() << "Usage:" << programName << "[options]";
    qDebug() << "Options:";
    qDebug() << "  --config <file>     Configuration file (JSON)";
    qDebug() << "  --step <1|2>        Pipeline step (1=generate, 2=analyze)";
    qDebug() << "  --output <dir>      Output directory";
    qDebug() << "  --tutorial <name>   Run neural network tutorial";
    qDebug() << "  --list-tutorials    List available tutorials";
    qDebug() << "  --interactive       Run tutorial in interactive mode";
    qDebug() << "  --help              Show this help";
    qDebug() << "";
    qDebug() << "Available Tutorials:";
    qDebug() << "  xor                 XOR problem demonstration (basic)";
    qDebug() << "  nmr                 NMR model selection (advanced chemistry)";
    qDebug() << "  training            Complete neural network training tutorial";
    qDebug() << "";
    qDebug() << "Tutorial Examples:";
    qDebug() << " " << programName << "--tutorial xor";
    qDebug() << " " << programName << "--tutorial xor --interactive";
    qDebug() << " " << programName << "--tutorial nmr";
    qDebug() << " " << programName << "--tutorial nmr --interactive";
    qDebug() << " " << programName << "--tutorial training";
    qDebug() << " " << programName << "--tutorial training --interactive";
}

void listTutorials()
{
    qDebug() << "Available Neural Network Tutorials:";
    qDebug() << "";
    qDebug() << "1. XOR Problem (xor) - Basic";
    qDebug() << "   - Demonstrates why neural networks need hidden layers";
    qDebug() << "   - Shows perceptron limitations on non-linear problems";
    qDebug() << "   - Educational step-by-step learning";
    qDebug() << "   - Pre-trained solution with mathematical explanations";
    qDebug() << "";
    qDebug() << "2. NMR Model Selection (nmr) - Advanced Chemistry";
    qDebug() << "   - Practical ML applications in analytical chemistry";
    qDebug() << "   - Feature engineering from NMR titration data";
    qDebug() << "   - Multi-class classification for binding models";
    qDebug() << "   - Real-world supramolecular chemistry examples";
    qDebug() << "";
    qDebug() << "3. Training Tutorial (training) - Complete Training Workflow";
    qDebug() << "   - Step-by-step neural network training process";
    qDebug() << "   - Loss functions and optimizers explained";
    qDebug() << "   - Hyperparameter effects demonstration";
    qDebug() << "   - Overfitting, underfitting, and validation";
    qDebug() << "   - Chemistry applications and model interpretation";
    qDebug() << "";
    qDebug() << "Usage: --tutorial <name> [--interactive]";
}

void runTutorial(const QString& tutorialName, bool interactive)
{
    if (tutorialName.toLower() == "xor") {
        qDebug() << "🎓 Starting XOR Neural Network Tutorial";
        qDebug() << "=====================================";
        
        XORTutorial::TutorialConfig config;
        config.interactive = interactive;
        config.show_matrices = true;
        config.show_calculations = true;
        config.step_by_step = interactive;
        config.detail_level = interactive ? TutorialLevel::Intermediate : TutorialLevel::Basic;
        
        XORTutorial tutorial(config);
        
        if (interactive) {
            tutorial.runInteractive();
        } else {
            tutorial.runFullTutorial();
        }
        
        // Show final validation
        qDebug() << "";
        if (tutorial.validateXORSolution()) {
            qDebug() << "✅ Tutorial validation: XOR solution is correct!";
        } else {
            qDebug() << "❌ Tutorial validation: XOR solution failed!";
        }
        
        // Export tutorial results
        QJsonObject results = tutorial.exportTutorialResults();
        qDebug() << "";
        qDebug() << "📊 Tutorial Summary:";
        qDebug() << "   Tutorial:" << results["tutorial_name"].toString();
        qDebug() << "   Architecture:" << results["network_architecture"].toString();
        qDebug() << "   Activations:" << results["activation_functions"].toString();
        qDebug() << "   Goal:" << results["educational_goal"].toString();
        
    } else if (tutorialName.toLower() == "nmr") {
        qDebug() << "🧪 Starting NMR Model Selection Tutorial";
        qDebug() << "=====================================";
        
        NMRModelSelectionTutorial::NMRTutorialConfig config;
        config.interactive = interactive;
        config.show_feature_engineering = true;
        config.show_model_comparison = true;
        config.show_chemical_interpretation = true;
        config.detail_level = interactive ? TutorialLevel::Advanced : TutorialLevel::Intermediate;
        
        NMRModelSelectionTutorial tutorial(config);
        
        if (interactive) {
            tutorial.runInteractive();
        } else {
            tutorial.runFullTutorial();
        }
        
        // Show final validation
        qDebug() << "";
        if (tutorial.validateModelSelection()) {
            qDebug() << "✅ Tutorial validation: NMR model selection is working!";
        } else {
            qDebug() << "❌ Tutorial validation: NMR model selection needs improvement!";
        }
        
        // Export tutorial results
        QJsonObject results = tutorial.exportNMRTutorialResults();
        qDebug() << "";
        qDebug() << "📊 Advanced Tutorial Summary:";
        qDebug() << "   Tutorial:" << results["tutorial_name"].toString();
        qDebug() << "   Architecture:" << results["network_architecture"].toString();
        qDebug() << "   Application:" << results["chemical_relevance"].toString();
        
    } else if (tutorialName.toLower() == "training") {
        qDebug() << "🎯 Starting Neural Network Training Tutorial";
        qDebug() << "===========================================";
        
        TrainingTutorial::TrainingTutorialConfig config;
        config.interactive = interactive;
        config.show_data_preparation = true;
        config.show_training_details = true;
        config.show_hyperparameter_effects = true;
        config.show_chemical_interpretation = true;
        config.detail_level = interactive ? TutorialLevel::Advanced : TutorialLevel::Intermediate;
        
        TrainingTutorial tutorial(config);
        
        if (interactive) {
            tutorial.runInteractive();
        } else {
            tutorial.runFullTutorial();
        }
        
        // Export tutorial results
        QJsonObject results = tutorial.exportTrainingResults();
        qDebug() << "";
        qDebug() << "📊 Training Tutorial Summary:";
        qDebug() << "   Tutorial completed:" << results["tutorial_completed"].toBool();
        qDebug() << "   Experiments run:" << results["experiments_run"].toInt();
        qDebug() << "   Training algorithms demonstrated: SGD, Momentum, Adam";
        qDebug() << "   Loss functions used: MSE, Cross-entropy, Binary Cross-entropy";
        
    } else {
        qDebug() << "❌ Unknown tutorial:" << tutorialName;
        qDebug() << "Available tutorials: xor, nmr, training";
        qDebug() << "Use --list-tutorials to see all available tutorials";
    }
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    QCommandLineParser parser;
    
    parser.addOption({{"c", "config"}, "Configuration file", "file"});
    parser.addOption({{"s", "step"}, "Pipeline step (1=generate, 2=analyze)", "step", "1"});
    parser.addOption({{"o", "output"}, "Output directory", "dir", "."});
    parser.addOption({{"t", "tutorial"}, "Run neural network tutorial", "name"});
    parser.addOption({"list-tutorials", "List available tutorials"});
    parser.addOption({{"i", "interactive"}, "Run tutorial in interactive mode"});
    parser.addOption({{"h", "help"}, "Show help"});
    
    parser.process(app);
    
    if (parser.isSet("help")) {
        printUsage(argv[0]);
        return 0;
    }
    
    if (parser.isSet("list-tutorials")) {
        listTutorials();
        return 0;
    }
    
    if (parser.isSet("tutorial")) {
        QString tutorialName = parser.value("tutorial");
        bool interactive = parser.isSet("interactive");
        
        runTutorial(tutorialName, interactive);
        return 0;
    }
    
    if (!parser.isSet("config")) {
        printUsage(argv[0]);
        return 1;
    }
    
    // Focus on Neural Network tutorials only - remove SupraFitCli dependencies
    qDebug() << "🧠 SupraFit ML Tutorial System";
    qDebug() << "Neural Network Learning Platform";
    return 0;
}