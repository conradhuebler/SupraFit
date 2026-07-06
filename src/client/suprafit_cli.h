/*
 * SupraFit Command Line Tools for batch processing and data generation
 * Copyright (C) 2018 - 2025 Conrad Hübler <Conrad.Huebler@gmx.net>
 * 
 * This CLI tool provides batch processing capabilities for SupraFit,
 * including modular data generation with Independent/Dependent structure,
 * file loading with range selection, and ML pipeline management.
 * Enhanced with modular structure by Claude Code AI Assistant.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <QtCore/QJsonObject>
#include <QtCore/QObject>
#include <QtCore/QVector>
#include <iostream>

#include "src/core/models/models.h"
#include "src/core/projectmanager.h"
#include "src/core/analysis_manager.h"

// Forward declaration to avoid multiple includes
class JobManager;

class SupraFitCli : public QObject {
    Q_OBJECT
public:
    SupraFitCli();
    SupraFitCli(SupraFitCli* other);
    ~SupraFitCli();

    void setControlJson(const QJsonObject& control);
    QJsonObject getControlJson() const { return m_main; }
    void setShowPostProcessingDetails(bool show) { m_show_post_processing_details = show; }

    void ParseMain();

    bool LoadFile();
    bool SaveFile();
    bool SaveFile(const QString& file, const QJsonObject& data);
    bool SaveFiles(const QString& file, const QVector<QJsonObject>& projects);

    void PrintFileContent(int index = 0);
    void PrintFileStructure();

    void Analyse(const QJsonObject& analyse, const QVector<QJsonObject>& models = QVector<QJsonObject>());

    inline void setInFile(const QString& file)
    {
        m_infile = file;
        m_outfile = file;
    }
    inline void setOutFile(const QString& file)
    {
        m_outfile = file;
        m_outfile.contains(".json") ? m_extension = ".json" : m_extension = ".suprafit";
        m_outfile.remove(".json").remove(".suprafit");
    }
    QVector<QJsonObject> Data() const { return SupraFit::ProjectManager::instance().getAllProjectsAsJson(); }

    void setDataJson(const QJsonObject& datajson) { m_data_json = datajson; }

    void OpenFile();

    // REFACTOR(D3-TaskRunner): Work / PerformeJobs / CheckStopFile are the CLI's job-execution
    // orchestration over JobManager. Extract to a shared TaskController when touched — the GUI
    // (metamodelwidget / modeldataholder) drives JobManager directly with parallel logic, so this
    // is the natural point to unify CLI+GUI job execution. See TECHNICAL_DEBT §D3 (GUI-overlap map).
    void Work();

    QJsonObject PerformeJobs(const QJsonObject& data, const QJsonObject& models, const QJsonObject& job);
    inline bool SimulationData() const { return m_simulate_job; }

    inline bool CheckGenerateIndependent() const { return m_generate_independent; }
    inline bool CheckGenerateDependent() const { return m_generate_dependent; }
    inline bool CheckGenerateNoisyDependent() { return m_generate_noisy_dependent; }
    inline bool CheckGenerateNoisyIndependent() { return m_generate_noisy_independent; }
    inline bool CheckDataOnly() const { return m_data_only; }
    inline bool CheckGenerateInputData() const { return m_generate_input_data; }

    void setDataVector(const QVector<QJsonObject>& data_vector)
    {
        // Store in legacy m_data_vector for compatibility
        m_data_vector = data_vector;

        // Also add all projects to ProjectManager
        SupraFit::ProjectManager& projectManager = SupraFit::ProjectManager::instance();
        projectManager.clearAllProjects(); // Clear existing projects first

        for (const QJsonObject& projectData : data_vector) {
            projectManager.createProjectFromJson(projectData, "CLI Generated Project");
        }
    }

    // REFACTOR(D3-DataFactory): these GenerateData* orchestrators + generateDependentDataTable
    // (below) are the m_*-coupled remainder of the DataFactory extraction — they need a config
    // bundle passed to DataFactory (not a verbatim lift). generateDependentDataTable also writes
    // m_modelContent/m_mlRawData → return a small result struct. When touched, finish moving them
    // into DataFactory; the GUI generatedatadialog wraps DataGenerator itself and should share
    // DataFactory here too. See TECHNICAL_DEBT §D3 (GUI-overlap map).
    QVector<QJsonObject> GenerateData();
    QVector<QJsonObject> GenerateDataOnly();
    QVector<QJsonObject> GenerateInputData();
    
    void AnalyzeFile();
    
    // AnalysisManager integration - Claude Generated
    void displayAnalysisResults(const QJsonObject& results);

    // Config analysis, model-statistics reporting and parameter extraction moved to
    // AnalysisReporter (D3) - Claude Generated

    // Enhanced methods using DataGenerator - Claude Generated
    QVector<QJsonObject> GenerateDataWithDataGenerator();
    
    // New modular JSON structure support - Claude Generated
    QVector<QJsonObject> GenerateDataWithModularStructure();
    bool parseModularStructure(const QJsonObject& control);
    QJsonObject generateDependentDataTable(const QJsonObject& dependentConfig, const QJsonObject& independentTableJson);
    QPointer<DataClass> generateIndependentData(const QJsonObject& independentConfig);
    QPointer<DataClass> generateDependentData(const QJsonObject& dependentConfig, QPointer<DataClass> independentData);
    QPointer<DataClass> loadDataFromFile(const QJsonObject& fileConfig);
    
    // REFACTOR(D3-MlPipeline): ProcessMLPipeline / FitModelsToData / EvaluateModelFit /
    // ExtractMLFeatures / runPostFitAnalysis are the CLI ML-pipeline cluster. FitModelsToData
    // duplicates AnalysisManager::fitModelsToData (the core, now really-fitting one) — extract to a
    // MlPipeline class that drives AnalysisManager, and drop the CLI-side duplicate. The GUI has no
    // ML pipeline but shares the fit/statistics substrate. See TECHNICAL_DEBT §D3 (GUI-overlap map).
    // ML Pipeline integration - Claude Generated
    /**
     * @brief Complete ML pipeline: data generation → model fitting → evaluation
     * @return Vector of project files with fitted models and statistical evaluation
     */
    QVector<QJsonObject> ProcessMLPipeline();
    
    /**
     * @brief Fit multiple models to simulated or real data for comparison
     * @param data DataClass containing independent/dependent data
     * @param modelsConfig JSON configuration specifying which models to test
     * @return Vector of fitted models with statistical metrics
     */
    QVector<QJsonObject> FitModelsToData(QPointer<DataClass> data, const QJsonObject& modelsConfig, const QJsonObject& globalAnalysisConfig = QJsonObject());

    /**
     * @brief Evaluate model fit quality and extract statistical features
     * @param model Fitted model to evaluate
     * @param trueModelId Original model ID used for data generation (if known)
     * @return JSON with fit statistics, AIC, SSE, R², and ML features
     */
    QJsonObject EvaluateModelFit(QSharedPointer<AbstractModel> model, int trueModelId = -1);
    
    /**
     * @brief Extract ML training features from fitted model for dataset generation
     * @param model Fitted model with calculated statistics
     * @param includeStatistics Include advanced statistical metrics (MC, CV)
     * @return JSON with features suitable for ML training
     */
    QJsonObject ExtractMLFeatures(QSharedPointer<AbstractModel> model, bool includeStatistics = true);

    /**
     * @brief Execute post-fit statistical analysis using JobManager (replaces DataGenerator logic)
     * @param model Fitted model to analyze
     * @param analysisConfig JSON configuration for analysis methods
     * @return JSON with analysis results from JobManager
     */
    QJsonObject runPostFitAnalysis(QSharedPointer<AbstractModel> model, const QJsonObject& analysisConfig);

    // ML training data export moved to MlExport (D3) - Claude Generated

    inline QString Extension() const { return m_extension; }
    inline QString OutFile() const { return m_outfile; }
signals:

public slots:
    void CheckStopFile();

protected:
    QSharedPointer<AbstractModel> AddModel(int model, QPointer<DataClass> data);
    QVector<QSharedPointer<AbstractModel>> AddModels(const QJsonObject& modelsjson, QPointer<DataClass> data);

    /**
     * @brief Bereinigt ein Projekt-JSON-Objekt für ML-spezifische Ausgabe
     * @param project Das ursprüngliche Projekt-Objekt
     * @return Bereinigtes QJsonObject, oder leeres Objekt wenn keine methods vorhanden
     *
     * Entfernt Rohdaten (independent/dependent) und komprimiert statistische
     * Methoden-Ergebnisse mittels extractCompactMLFeatures().
     *
     * Claude Generated - 2025-10-17
     */
    QJsonObject cleanProjectForML(const QJsonObject& project) const;

    QString m_infile = QString();
    QString m_outfile = QString(), m_extension = ".suprafit";

    /* Controller json */
    QJsonObject m_main, m_jobs, m_models, m_analyse;

    /* Sub json */
    QJsonObject m_prepare, m_simulation;
    
    /* Original configuration - Claude Generated */
    QJsonObject m_original_config;
    
    /* Modular structure support - Claude Generated */
    QJsonObject m_independent, m_dependent;
    bool m_use_modular_structure = false;
    bool m_show_post_processing_details = false;

    /* Stored data structure */
    QJsonObject m_data_json;

    // m_toplevel/m_data removed - project state is read directly from ProjectManager.
    // m_data_vector retained for CLI multi-project export. - Claude Generated
    QVector<QJsonObject> m_data_vector;

    // JobManager for statistical analysis - Claude Generated
    JobManager* m_jobmanager;
    
    // AnalysisManager for centralized analysis - Claude Generated
    AnalysisManager* m_analysisManager;

    QString m_modelContent; // Store enhanced content from model generation - Claude Generated
    QJsonObject m_mlRawData; // Store ML RawData from model generation - Claude Generated
    
    // ML Training Data Export Configuration - Claude Generated
    bool m_exportMLTraining = false;
    bool m_autoExportML = false;
    QString m_mlOutputFile = "";
    QString m_mlBatchDirectory = "";

    int m_independent_rows = 2, m_start_point = 0, m_series = 0;
    bool m_guess = false, m_fit = false;
    bool m_simulate_job = false;

    bool m_generate_independent = false;
    bool m_generate_dependent = false;
    bool m_generate_noisy_independent = false;
    bool m_generate_noisy_dependent = false;
    bool m_data_only = false;
    bool m_generate_input_data = false;

    bool m_interrupt = false;

signals:
    void Interrupt();
};
