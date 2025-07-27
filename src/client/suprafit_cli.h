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

#include "src/core/models/models.h"

class SupraFitCli : public QObject {
    Q_OBJECT
public:
    SupraFitCli();
    SupraFitCli(SupraFitCli* other);
    ~SupraFitCli();

    void setControlJson(const QJsonObject& control);

    void ParseMain();
    void ParsePrepare();

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
    QVector<QJsonObject> Data() const { return QVector<QJsonObject>() << m_toplevel; }

    void setDataJson(const QJsonObject& datajson) { m_data_json = datajson; }

    void OpenFile();

    void Work();

    QJsonObject PerformeJobs(const QJsonObject& data, const QJsonObject& models, const QJsonObject& job);
    inline bool SimulationData() const { return m_simulate_job; }

    inline bool CheckGenerateIndependent() const { return m_generate_independent; }
    inline bool CheckGenerateDependent() const { return m_generate_dependent; }
    inline bool CheckGenerateNoisyDependent() { return m_generate_noisy_dependent; }
    inline bool CheckGenerateNoisyIndependent() { return m_generate_noisy_independent; }
    inline bool CheckDataOnly() const { return m_data_only; }
    inline bool CheckGenerateInputData() const { return m_generate_input_data; }

    void setDataVector(const QVector<QJsonObject>& data_vector) { m_data_vector = data_vector; }

    QVector<QJsonObject> GenerateData();
    QVector<QJsonObject> GenerateDataOnly();
    QVector<QJsonObject> GenerateInputData();
    
    void AnalyzeFile();

    // Enhanced analysis methods for read-only mode - Claude Generated
    void analyzeGenerateDataConfig(const QJsonObject& generateDataConfig);
    void validateGenerateDataConfig(const QJsonObject& config);

    // Enhanced methods using DataGenerator - Claude Generated
    QVector<QJsonObject> GenerateDataWithDataGenerator();
    bool validateDataGeneratorConfig(const QJsonObject& config) const;
    
    // New modular JSON structure support - Claude Generated
    QVector<QJsonObject> GenerateDataWithModularStructure();
    bool parseModularStructure(const QJsonObject& control);
    QJsonObject generateIndependentDataTable(const QJsonObject& independentConfig);
    QJsonObject generateDependentDataTable(const QJsonObject& dependentConfig, const QJsonObject& independentTableJson);
    QJsonObject loadDataTableFromFile(const QJsonObject& fileConfig);
    QPointer<DataClass> generateIndependentData(const QJsonObject& independentConfig);
    QPointer<DataClass> generateDependentData(const QJsonObject& dependentConfig, QPointer<DataClass> independentData);
    QPointer<DataClass> loadDataFromFile(const QJsonObject& fileConfig);
    QPointer<DataClass> applyNoise(QPointer<DataClass> data, const QJsonObject& noiseConfig, bool isIndependent);
    
    inline QString Extension() const { return m_extension; }
    inline QString OutFile() const { return m_outfile; }
signals:

public slots:
    void CheckStopFile();

protected:
    QSharedPointer<AbstractModel> AddModel(int model, QPointer<DataClass> data);
    QVector<QSharedPointer<AbstractModel>> AddModels(const QJsonObject& modelsjson, QPointer<DataClass> data);

    QString m_infile = QString();
    QString m_outfile = QString(), m_extension = "json";

    /* Controller json */
    QJsonObject m_main, m_jobs, m_models, m_analyse;

    /* Sub json */
    QJsonObject m_prepare, m_simulation;
    
    /* Modular structure support - Claude Generated */
    QJsonObject m_independent, m_dependent;
    bool m_use_modular_structure = false;

    /* Stored data structure */
    QJsonObject m_data_json;

    QVector<QJsonObject> m_data_vector;

    /* Json to be written to a file */
    QJsonObject m_toplevel;

    QPointer<DataClass> m_data;

    QString m_modelContent; // Store enhanced content from model generation - Claude Generated

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
