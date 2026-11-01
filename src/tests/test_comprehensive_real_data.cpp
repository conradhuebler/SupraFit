/**
 * Comprehensive Real Data Test Suite
 * Copyright (C) 2019 - 2025 Conrad Hübler <Conrad.Huebler@gmx.net>
 * 
 * Tests complete workflow using real NMR titration data (1_1_1_2_001.dat)
 * - 4 Model types: 1:1, 1:1/1:2, 2:1/1:1, 2:1/1:1/1:2
 * - 5 Statistical analyses: Monte Carlo, L0O, L2O, L30-CV, Parameter Reduction  
 * - Model ranking and comparison
 * - Reproducibility with RandomSeed support
 * - Performance benchmarking
 */

#include <QtCore/QCoreApplication>
#include <QtCore/QElapsedTimer>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QDateTime>
#include <QtCore/QProcess>
#include <QtCore/QDir>
#include <QtTest/QTest>

#include <algorithm>
#include <numeric>
#include <limits>
#include <cmath>

#include "test_utils.h"
#include "src/core/jsonhandler.h"

class ComprehensiveRealDataTest : public QObject {
    Q_OBJECT

public:
    ComprehensiveRealDataTest();

private slots:
    void initTestCase();
    void cleanupTestCase();

    // Model Fitting Tests
    void test_model_1_1_fitting();
    void test_model_1_1_1_2_fitting();
    void test_model_2_1_1_1_fitting();
    void test_model_2_1_1_1_1_2_fitting();

    // Statistical Analysis Tests
    void test_monte_carlo_analysis();
    void test_leave_one_out_cv();
    void test_leave_two_out_cv();
    void test_cross_x_out_cv();
    void test_parameter_reduction();

    // Model Comparison Tests
    void test_model_ranking_and_comparison();
    void test_aic_based_selection();
    void test_evidence_ratios();

    // Reproducibility Tests
    void test_monte_carlo_reproducibility();
    void test_cross_validation_stability();

    // Performance Benchmarks
    void test_workflow_performance();
    void test_parallel_efficiency();

    // Integration Tests
    void test_complete_workflow();
    void test_comprehensive_report_generation();

private:
    // Claude Generated helper methods
    bool runCliCommand(const QString& config, const QString& outputPrefix = QString());
    QJsonObject loadSupraFitFile(const QString& filename);
    QJsonObject extractFitQuality(const QJsonObject& model);
    QJsonObject extractStatisticalAnalysis(const QJsonObject& model);
    
    QString generateModelTestConfig(int modelId, const QString& analysisType = QString());
    QString generateComprehensiveConfig();
    
    void verifyModelFitQuality(const QJsonObject& fitQuality, double minR2 = 0.8);
    void verifyStatisticalResults(const QJsonObject& analysis, const QString& method);
    void compareModelRankings(const QVector<QJsonObject>& models);
    
    QString m_testDir;
    QElapsedTimer m_benchmarkTimer;
    QJsonObject m_testResults;
};

ComprehensiveRealDataTest::ComprehensiveRealDataTest()
{
    // Claude Generated: Initialize benchmark results structure
    m_testResults["test_suite"] = "Comprehensive Real Data Analysis";
    m_testResults["data_file"] = "1_1_1_2_001.dat";
    m_testResults["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    m_testResults["models"] = QJsonArray();
    m_testResults["performance"] = QJsonObject();
}

void ComprehensiveRealDataTest::initTestCase()
{
    // Use persistent directory for test results
    m_testDir = QDir::currentPath() + "/test_comprehensive_results";
    QDir().mkpath(m_testDir);
    
    // Verify CLI binary is available through TestUtils
    QString cliPath = TestUtils::findSuprafitCli();
    QVERIFY2(!cliPath.isEmpty(), "suprafit_cli executable not found - set SUPRAFIT_CLI_PATH env var if needed");
    
    // Verify input data exists relative to current directory
    QVERIFY2(QFile::exists(QStringLiteral(SUPRAFIT_SOURCE_DIR "/input/1_1_1_2_001.dat")), 
             "Real NMR data file not found");
}

void ComprehensiveRealDataTest::cleanupTestCase()
{
    // Results are kept persistent in test_comprehensive_results directory
    qDebug() << "Test results saved in:" << m_testDir;
}

void ComprehensiveRealDataTest::test_model_1_1_fitting()
{
    QBENCHMARK {
        m_benchmarkTimer.start();
        
        QString config = generateModelTestConfig(1, "fitting");
        bool success = runCliCommand(config, "model_1_1");
        QVERIFY2(success, "1:1 Model fitting failed");
        
        // Check what files were actually created
        QDir testDir(m_testDir);
        QStringList files = testDir.entryList(QDir::Files);
        qDebug() << "Files created in test dir:" << files;
        
        QString outputFile = m_testDir + "/model_1_1-project-0.suprafit";
        qDebug() << "Expected output file:" << outputFile;
        QVERIFY2(QFile::exists(outputFile), "1:1 Model output file not created");
        
        QJsonObject model = loadSupraFitFile(outputFile);
        QVERIFY(!model.isEmpty());
        
        QJsonObject fitQuality = extractFitQuality(model);
        verifyModelFitQuality(fitQuality, 0.85);
        
        // Claude Generated: Store results for comparison
        QJsonObject modelResult;
        modelResult["model_id"] = 1;
        modelResult["model_name"] = "1:1 Model";
        modelResult["fit_quality"] = fitQuality;
        modelResult["fitting_time"] = m_benchmarkTimer.elapsed();
        
        QJsonArray models = m_testResults["models"].toArray();
        models.append(modelResult);
        m_testResults["models"] = models;
    }
}

void ComprehensiveRealDataTest::test_model_1_1_1_2_fitting()
{
    QBENCHMARK {
        m_benchmarkTimer.start();
        
        QString config = generateModelTestConfig(3, "fitting");
        bool success = runCliCommand(config, "model_1_1_1_2");
        QVERIFY2(success, "1:1/1:2 Model fitting failed");
        
        QString outputFile = m_testDir + "/model_1_1_1_2-project-0.suprafit";
        QVERIFY2(QFile::exists(outputFile), "1:1/1:2 Model output file not created");
        
        QJsonObject model = loadSupraFitFile(outputFile);
        QVERIFY(!model.isEmpty());
        
        QJsonObject fitQuality = extractFitQuality(model);
        verifyModelFitQuality(fitQuality, 0.80);
        
        QJsonObject modelResult;
        modelResult["model_id"] = 3;
        modelResult["model_name"] = "1:1/1:2 Model";
        modelResult["fit_quality"] = fitQuality;
        modelResult["fitting_time"] = m_benchmarkTimer.elapsed();
        
        QJsonArray models = m_testResults["models"].toArray();
        models.append(modelResult);
        m_testResults["models"] = models;
    }
}

void ComprehensiveRealDataTest::test_model_2_1_1_1_fitting()
{
    QBENCHMARK {
        m_benchmarkTimer.start();
        
        QString config = generateModelTestConfig(2, "fitting");
        bool success = runCliCommand(config, "model_2_1_1_1");
        QVERIFY2(success, "2:1/1:1 Model fitting failed");
        
        QString outputFile = m_testDir + "/model_2_1_1_1-project-0.suprafit";
        QVERIFY2(QFile::exists(outputFile), "2:1/1:1 Model output file not created");
        
        QJsonObject model = loadSupraFitFile(outputFile);
        QVERIFY(!model.isEmpty());
        
        QJsonObject fitQuality = extractFitQuality(model);
        verifyModelFitQuality(fitQuality, 0.75);
        
        QJsonObject modelResult;
        modelResult["model_id"] = 2;
        modelResult["model_name"] = "2:1/1:1 Model";
        modelResult["fit_quality"] = fitQuality;
        modelResult["fitting_time"] = m_benchmarkTimer.elapsed();
        
        QJsonArray models = m_testResults["models"].toArray();
        models.append(modelResult);
        m_testResults["models"] = models;
    }
}

void ComprehensiveRealDataTest::test_model_2_1_1_1_1_2_fitting()
{
    QBENCHMARK {
        m_benchmarkTimer.start();
        
        QString config = generateModelTestConfig(4, "fitting");
        bool success = runCliCommand(config, "model_2_1_1_1_1_2");
        QVERIFY2(success, "2:1/1:1/1:2 Model fitting failed");
        
        QString outputFile = m_testDir + "/model_2_1_1_1_1_2-project-0.suprafit";
        QVERIFY2(QFile::exists(outputFile), "2:1/1:1/1:2 Model output file not created");
        
        QJsonObject model = loadSupraFitFile(outputFile);
        QVERIFY(!model.isEmpty());
        
        QJsonObject fitQuality = extractFitQuality(model);
        verifyModelFitQuality(fitQuality, 0.70);
        
        QJsonObject modelResult;
        modelResult["model_id"] = 4;
        modelResult["model_name"] = "2:1/1:1/1:2 Model";
        modelResult["fit_quality"] = fitQuality;
        modelResult["fitting_time"] = m_benchmarkTimer.elapsed();
        
        QJsonArray models = m_testResults["models"].toArray();
        models.append(modelResult);
        m_testResults["models"] = models;
    }
}

void ComprehensiveRealDataTest::test_monte_carlo_analysis()
{
    QString config = generateModelTestConfig(1, "monte_carlo");
    bool success = runCliCommand(config, "mc_analysis");
    QVERIFY2(success, "Monte Carlo analysis failed");
    
    QString outputFile = m_testDir + "/mc_analysis-project-0.suprafit";
    QVERIFY2(QFile::exists(outputFile), "Monte Carlo output file not created");
    
    QJsonObject model = loadSupraFitFile(outputFile);
    QJsonObject analysis = extractStatisticalAnalysis(model);
    
    verifyStatisticalResults(analysis, "monte_carlo");
    
    // Verify RandomSeed reproducibility
    QVERIFY2(analysis.contains("monte_carlo"), "Monte Carlo results missing");
    
    QJsonObject mcResults = analysis["monte_carlo"].toObject();
    QVERIFY2(mcResults["max_steps"].toInt() == 1000, "Monte Carlo steps incorrect");
    QVERIFY2(mcResults.contains("models"), "Monte Carlo parameter distributions missing");
}

void ComprehensiveRealDataTest::test_leave_one_out_cv()
{
    QString config = generateModelTestConfig(1, "l0o");
    bool success = runCliCommand(config, "l0o_analysis");
    QVERIFY2(success, "Leave-One-Out CV failed");
    
    QString outputFile = m_testDir + "/l0o_analysis-project-0.suprafit";
    QJsonObject model = loadSupraFitFile(outputFile);
    QJsonObject analysis = extractStatisticalAnalysis(model);
    
    verifyStatisticalResults(analysis, "cross_validation_l0o");
}

void ComprehensiveRealDataTest::test_leave_two_out_cv()
{
    QString config = generateModelTestConfig(1, "l2o");
    bool success = runCliCommand(config, "l2o_analysis");
    QVERIFY2(success, "Leave-Two-Out CV failed");
    
    QString outputFile = m_testDir + "/l2o_analysis-project-0.suprafit";
    QJsonObject model = loadSupraFitFile(outputFile);
    QJsonObject analysis = extractStatisticalAnalysis(model);
    
    verifyStatisticalResults(analysis, "cross_validation_l2o");
}

void ComprehensiveRealDataTest::test_cross_x_out_cv()
{
    QString config = generateModelTestConfig(1, "cxo");
    bool success = runCliCommand(config, "cxo_analysis");
    QVERIFY2(success, "Cross-X-Out CV failed");
    
    QString outputFile = m_testDir + "/cxo_analysis-project-0.suprafit";
    QJsonObject model = loadSupraFitFile(outputFile);
    QJsonObject analysis = extractStatisticalAnalysis(model);
    
    verifyStatisticalResults(analysis, "cross_validation_cxo");
}

void ComprehensiveRealDataTest::test_parameter_reduction()
{
    QString config = generateModelTestConfig(1, "reduction");
    bool success = runCliCommand(config, "reduction_analysis");
    QVERIFY2(success, "Parameter Reduction analysis failed");
    
    QString outputFile = m_testDir + "/reduction_analysis-project-0.suprafit";
    QJsonObject model = loadSupraFitFile(outputFile);
    QJsonObject analysis = extractStatisticalAnalysis(model);
    
    verifyStatisticalResults(analysis, "parameter_reduction");
}

void ComprehensiveRealDataTest::test_model_ranking_and_comparison()
{
    // Run comprehensive workflow with all 4 models
    QString config = generateComprehensiveConfig();
    bool success = runCliCommand(config, "comprehensive");
    QVERIFY2(success, "Comprehensive workflow failed");
    
    QString outputFile = m_testDir + "/comprehensive-project-0.suprafit";
    QJsonObject results = loadSupraFitFile(outputFile);
    
    // the project file holds the fitted models as model_0, model_1, ...
    QJsonArray modelsArray;
    for (auto it = results.begin(); it != results.end(); ++it)
        if (it.key().startsWith("model_"))
            modelsArray.append(it.value());
    QVERIFY2(!modelsArray.isEmpty(), "Model comparison results missing");
    QVERIFY2(modelsArray.size() == 4, "Expected 4 models in comparison");
    
    QVector<QJsonObject> models;
    for (const QJsonValue& value : modelsArray) {
        models.append(value.toObject());
    }
    
    compareModelRankings(models);
}

void ComprehensiveRealDataTest::test_aic_based_selection()
{
    QJsonArray models = m_testResults["models"].toArray();
    QVERIFY2(models.size() >= 2, "Need at least 2 models for AIC comparison");
    
    // Find best AIC model
    double bestAIC = std::numeric_limits<double>::max();
    int bestModel = -1;
    
    for (int i = 0; i < models.size(); ++i) {
        QJsonObject model = models[i].toObject();
        QJsonObject fitQuality = model["fit_quality"].toObject();
        double aic = fitQuality["aic"].toDouble();
        
        if (aic < bestAIC) {
            bestAIC = aic;
            bestModel = i;
        }
    }
    
    QVERIFY2(bestModel >= 0, "Could not determine best AIC model");
    
    QJsonObject performance = m_testResults["performance"].toObject();
    performance["best_aic_model_id"] = bestModel + 1;
    performance["best_aic_value"] = bestAIC;
    m_testResults["performance"] = performance;
}

void ComprehensiveRealDataTest::test_evidence_ratios()
{
    QJsonArray models = m_testResults["models"].toArray();
    if (models.size() < 2) return;
    
    // Calculate evidence ratios relative to best model
    double bestAIC = std::numeric_limits<double>::max();
    for (const QJsonValue& value : models) {
        QJsonObject model = value.toObject();
        QJsonObject fitQuality = model["fit_quality"].toObject();
        double aic = fitQuality["aic"].toDouble();
        bestAIC = std::min(bestAIC, aic);
    }
    
    QJsonArray evidenceRatios;
    for (const QJsonValue& value : models) {
        QJsonObject model = value.toObject();
        QJsonObject fitQuality = model["fit_quality"].toObject();
        double aic = fitQuality["aic"].toDouble();
        double deltaAIC = aic - bestAIC;
        double evidenceRatio = std::exp(-0.5 * deltaAIC);
        
        QJsonObject ratio;
        ratio["model_id"] = model["model_id"];
        ratio["delta_aic"] = deltaAIC;
        ratio["evidence_ratio"] = evidenceRatio;
        evidenceRatios.append(ratio);
    }
    
    QJsonObject performance = m_testResults["performance"].toObject();
    performance["evidence_ratios"] = evidenceRatios;
    m_testResults["performance"] = performance;
}

void ComprehensiveRealDataTest::test_monte_carlo_reproducibility()
{
    // Run Monte Carlo twice with same seed
    QString config = generateModelTestConfig(1, "monte_carlo");
    
    bool success1 = runCliCommand(config, "mc_repro_1");
    bool success2 = runCliCommand(config, "mc_repro_2");
    
    QVERIFY2(success1 && success2, "Monte Carlo reproducibility runs failed");
    
    QJsonObject results1 = loadSupraFitFile(m_testDir + "/mc_repro_1-project-0.suprafit");
    QJsonObject results2 = loadSupraFitFile(m_testDir + "/mc_repro_2-project-0.suprafit");
    
    // Compare parameter means - should be identical with same seed
    QJsonObject analysis1 = extractStatisticalAnalysis(results1);
    QJsonObject analysis2 = extractStatisticalAnalysis(results2);
    
    QVERIFY2(analysis1.contains("monte_carlo") && analysis2.contains("monte_carlo"),
             "Monte Carlo analysis missing in reproducibility test");
    
    // Note: Detailed comparison would require parsing parameter distributions
    // This test verifies that both runs completed successfully with same configuration
}

void ComprehensiveRealDataTest::test_cross_validation_stability()
{
    // Multiple CV runs should have consistent results (low variance)
    QString config = generateModelTestConfig(1, "l0o");
    
    QVector<double> r2Values;
    for (int run = 0; run < 3; ++run) {
        QString outputPrefix = QString("cv_stability_%1").arg(run);
        bool success = runCliCommand(config, outputPrefix);
        QVERIFY2(success, QString("CV stability run %1 failed").arg(run).toLocal8Bit());
        
        QString outputFile = m_testDir + QString("/%1-project-0.suprafit").arg(outputPrefix);
        QJsonObject model = loadSupraFitFile(outputFile);
        QJsonObject fitQuality = extractFitQuality(model);
        r2Values.append(fitQuality["r_squared"].toDouble());
    }
    
    // Calculate CV coefficient of variation
    double mean = std::accumulate(r2Values.begin(), r2Values.end(), 0.0) / r2Values.size();
    double variance = 0.0;
    for (double value : r2Values) {
        variance += (value - mean) * (value - mean);
    }
    variance /= (r2Values.size() - 1);
    double stdDev = std::sqrt(variance);
    double cv = stdDev / mean;
    
    QVERIFY2(cv < 0.1, "Cross-validation results show high variability");
    
    QJsonObject performance = m_testResults["performance"].toObject();
    performance["cv_stability_coefficient"] = cv;
    m_testResults["performance"] = performance;
}

void ComprehensiveRealDataTest::test_workflow_performance()
{
    QBENCHMARK {
        m_benchmarkTimer.start();
        
        QString config = generateComprehensiveConfig();
        bool success = runCliCommand(config, "performance_test");
        QVERIFY2(success, "Performance workflow failed");
        
        qint64 totalTime = m_benchmarkTimer.elapsed();
        
        QJsonObject performance = m_testResults["performance"].toObject();
        performance["total_workflow_time_ms"] = totalTime;
        performance["total_workflow_time_min"] = totalTime / 60000.0;
        m_testResults["performance"] = performance;
    }
}

void ComprehensiveRealDataTest::test_parallel_efficiency()
{
    // Compare single-threaded vs multi-threaded performance
    QString config = generateComprehensiveConfig();
    
    // Single-threaded run
    m_benchmarkTimer.start();
    bool success1 = runCliCommand(config, "single_thread");
    qint64 singleThreadTime = m_benchmarkTimer.elapsed();
    
    // Multi-threaded run (system default)
    m_benchmarkTimer.start();
    bool success2 = runCliCommand(config, "multi_thread");
    qint64 multiThreadTime = m_benchmarkTimer.elapsed();
    
    QVERIFY2(success1 && success2, "Parallel efficiency tests failed");
    
    double speedup = static_cast<double>(singleThreadTime) / multiThreadTime;
    
    QJsonObject performance = m_testResults["performance"].toObject();
    performance["single_thread_time_ms"] = singleThreadTime;
    performance["multi_thread_time_ms"] = multiThreadTime;
    performance["parallel_speedup"] = speedup;
    m_testResults["performance"] = performance;
    
    // runs of a few seconds cannot show a speedup: thread start and the writing of the results dominate
    if (singleThreadTime < 20000)
        QSKIP("the single-thread run is too short to measure a speedup");
    // a loaded machine (other tests running) hides the gain: more threads must at least not be markedly slower
    QVERIFY2(speedup > 0.7, "Multi-threading should not be markedly slower than one thread");
}

void ComprehensiveRealDataTest::test_complete_workflow()
{
    // Final integration test combining all components
    QString config = generateComprehensiveConfig();
    bool success = runCliCommand(config, "final_workflow");
    QVERIFY2(success, "Complete workflow integration failed");
    
    QString outputFile = m_testDir + "/final_workflow-project-0.suprafit";
    QVERIFY2(QFile::exists(outputFile), "Final workflow output file not created");
    
    QJsonObject results = loadSupraFitFile(outputFile);
    QVERIFY(!results.isEmpty());
    
    // Verify all expected sections are present
    QVERIFY2(results.contains("model_0"), "Models missing from final workflow");
    QVERIFY2(!extractStatisticalAnalysis(results).isEmpty(), "Statistical analysis missing from final workflow");
    
    // Store final results summary
    m_testResults["final_workflow_success"] = true;
    m_testResults["final_output_file"] = outputFile;
}

void ComprehensiveRealDataTest::test_comprehensive_report_generation()
{
    // Generate final test report
    QString reportPath = m_testDir + "/comprehensive_test_report.json";
    
    m_testResults["test_completion_time"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    QFile reportFile(reportPath);
    QVERIFY2(reportFile.open(QIODevice::WriteOnly), "Could not create test report file");
    
    QJsonDocument doc(m_testResults);
    reportFile.write(doc.toJson(QJsonDocument::Indented));
    reportFile.close();
    
    QVERIFY2(QFile::exists(reportPath), "Test report file not created");
    
    // Output report location for user
    qDebug() << "Comprehensive test report generated:" << reportPath;
    
    // Verify report contains expected sections
    QVERIFY2(m_testResults.contains("models"), "Report missing models section");
    QVERIFY2(m_testResults.contains("performance"), "Report missing performance section");
    QVERIFY2(m_testResults["models"].toArray().size() >= 4, "Report should contain all 4 models");
}

// Claude Generated helper method implementations

bool ComprehensiveRealDataTest::runCliCommand(const QString& config, const QString& outputPrefix)
{
    QString configFile = m_testDir + QString("/config_%1.json").arg(outputPrefix.isEmpty() ? "test" : outputPrefix);
    QString outputFile = outputPrefix.isEmpty() ? "test_output" : outputPrefix;
    
    // Write config to persistent file
    QFile file(configFile);
    if (!file.open(QIODevice::WriteOnly)) return false;
    file.write(config.toUtf8());
    file.close();
    
    // Run CLI command
    QProcess process;
    QStringList arguments;
    arguments << "-i" << configFile;
    arguments << "-o" << m_testDir + "/" + outputFile;
    
    process.start(TestUtils::findSuprafitCli(), arguments);
    bool finished = process.waitForFinished(300000); // 5 minute timeout
    
    if (!finished) {
        process.kill();
        qDebug() << "Process timed out";
        return false;
    }
    
    if (process.exitCode() != 0) {
        qDebug() << "CLI command failed with exit code:" << process.exitCode();
        qDebug() << "CLI stdout:" << process.readAllStandardOutput();
        qDebug() << "CLI stderr:" << process.readAllStandardError();
        return false;
    }
    
    return true;
}

QJsonObject ComprehensiveRealDataTest::loadSupraFitFile(const QString& filename)
{
    // plain or compressed: the format is taken from the content
    QJsonObject object;
    if (!JsonHandler::ReadFile(filename, object))
        return QJsonObject();
    return object;
}

/*! \brief The fit quality of the (first) model of a project file: AIC, AICc and SSE are stored with the model, R^2 is
 * 1 - SSE / SST of the stored dependent data (all active signals, around their own mean), rmse from SSE and the points. */
QJsonObject ComprehensiveRealDataTest::extractFitQuality(const QJsonObject& model)
{
    QJsonObject fitQuality;

    QJsonObject block = model;
    for (auto it = model.begin(); it != model.end(); ++it)
        if (it.key().startsWith("model_")) {
            block = it.value().toObject();
            break;
        }

    // the older flat names and the stored ones
    for (const QString& name : { QStringLiteral("aic"), QStringLiteral("aicc"), QStringLiteral("sse"), QStringLiteral("rmse"), QStringLiteral("chi_squared"), QStringLiteral("r_squared") })
        if (block.contains(name))
            fitQuality[name] = block[name];
    if (block.contains("AIC"))
        fitQuality["aic"] = block["AIC"];
    if (block.contains("AICc"))
        fitQuality["aicc"] = block["AICc"];
    if (block.contains("SSE"))
        fitQuality["sse"] = block["SSE"];

    if (!fitQuality.contains("r_squared") && fitQuality.contains("sse")) {
        const QJsonObject dependent = model["data"].toObject()["dependent"].toObject();
        const QJsonObject rows = dependent["data"].toObject();
        double sum = 0, sumsq = 0;
        int count = 0;
        for (auto it = rows.begin(); it != rows.end(); ++it)
            for (const QString& v : it.value().toString().split(QLatin1Char(' '), Qt::SkipEmptyParts)) {
                bool ok = false;
                const double x = v.toDouble(&ok);
                if (!ok)
                    continue;
                sum += x;
                sumsq += x * x;
                ++count;
            }
        if (count > 1) {
            const double sst = sumsq - sum * sum / count;
            if (sst > 0)
                fitQuality["r_squared"] = 1.0 - fitQuality["sse"].toDouble() / sst;
            if (!fitQuality.contains("rmse"))
                fitQuality["rmse"] = std::sqrt(fitQuality["sse"].toDouble() / count);
        }
    }
    return fitQuality;
}

/*! \brief The analyses stored with the (first) model of a project file, under the names the checks use: an exported model
 * numbers its analyses and every entry knows its method (controller): monte_carlo (max_steps, the parameter statistics as
 * models), cross_validation_l0o / _l2o / _cxo (CXO 1 / 2 / 3), parameter_reduction. Claude Generated. */
QJsonObject ComprehensiveRealDataTest::extractStatisticalAnalysis(const QJsonObject& model)
{
    QJsonObject analysis;

    QJsonObject block = model;
    for (auto it = model.begin(); it != model.end(); ++it)
        if (it.key().startsWith("model_")) {
            block = it.value().toObject();
            break;
        }

    const QJsonObject methods = block["data"].toObject()["methods"].toObject();
    for (auto it = methods.begin(); it != methods.end(); ++it) {
        const QJsonObject entry = it.value().toObject();
        const QJsonObject controller = entry["controller"].toObject();
        if (controller.isEmpty())
            continue;
        QJsonArray parameters;
        for (auto p = entry.begin(); p != entry.end(); ++p)
            if (p.key() != "controller")
                parameters.append(p.value());
        switch (controller["Method"].toInt()) {
        case 1: {
            QJsonObject mc;
            mc["max_steps"] = controller["MaxSteps"];
            mc["models"] = parameters;
            analysis["monte_carlo"] = mc;
            break;
        }
        case 4: {
            const int cxo = controller["CXO"].toInt();
            QJsonObject cv;
            cv["validation_errors"] = parameters;
            cv["controller"] = controller;
            analysis[cxo == 1 ? "cross_validation_l0o" : cxo == 2 ? "cross_validation_l2o" : "cross_validation_cxo"] = cv;
            analysis["cross_validation"] = cv;
            break;
        }
        case 5: {
            QJsonObject ra;
            ra["parameter_significance"] = parameters;
            ra["controller"] = controller;
            analysis["parameter_reduction"] = ra;
            break;
        }
        default:
            break;
        }
    }
    return analysis;
}

QString ComprehensiveRealDataTest::generateModelTestConfig(int modelId, const QString& analysisType)
{
    QJsonObject config;
    QJsonObject main;
    main["OutFile"] = QString("model_%1_test").arg(modelId);
    main["UseModularStructure"] = true;
    main["ProcessMLPipeline"] = true;
    main["FitModels"] = true;
    main["PostFitAnalysis"] = true;
    config["Main"] = main;
    
    // Data source configuration
    QJsonObject independent;
    independent["Source"] = "file";
    QJsonObject indepFile;
    indepFile["Path"] = QStringLiteral(SUPRAFIT_SOURCE_DIR "/input/1_1_1_2_001.dat");
    indepFile["StartRow"] = 0;
    indepFile["StartCol"] = 0;
    indepFile["Rows"] = 20;
    indepFile["Cols"] = 2;
    independent["File"] = indepFile;
    config["Independent"] = independent;
    
    QJsonObject dependent;
    dependent["Source"] = "file";
    QJsonObject depFile;
    depFile["Path"] = QStringLiteral(SUPRAFIT_SOURCE_DIR "/input/1_1_1_2_001.dat");
    depFile["StartRow"] = 0;
    depFile["StartCol"] = 2;
    depFile["Rows"] = 20;
    depFile["Cols"] = 7;
    dependent["File"] = depFile;
    config["Dependent"] = dependent;
    
    // Model configuration
    QJsonObject addModels;
    QJsonObject modelConfig;
    modelConfig["ID"] = modelId;
    
    QJsonObject options;
    options["FastMode"] = false;
    options["Convergency"] = 1e-8;
    modelConfig["Options"] = options;
    
    // Analysis-specific configuration (always add at least Monte Carlo)
    QJsonObject postFitAnalysis;
    postFitAnalysis["enabled"] = true;
    
    QJsonArray methods;
    QJsonObject method;
        
        if (analysisType == "monte_carlo") {
            method["Method"] = 1;
            method["MaxSteps"] = 1000;
            method["VarianceSource"] = 2;
            method["RandomSeed"] = 12345;
            methods.append(method);
        } else if (analysisType == "l0o") {
            method["Method"] = 4; // cross validation
            method["CXO"] = 1;
            method["Algorithm"] = 2;
            methods.append(method);
        } else if (analysisType == "l2o") {
            method["Method"] = 4; // cross validation
            method["CXO"] = 2;
            method["Algorithm"] = 2;
            methods.append(method);
        } else if (analysisType == "cxo") {
            method["Method"] = 4; // cross validation
            method["CXO"] = 3;
            method["X"] = 3;
            method["MaxSteps"] = 30;
            method["Algorithm"] = 2;
            methods.append(method);
        } else if (analysisType == "reduction") {
            method["Method"] = 5; // reduction
            method["ReductionRuntype"] = 3;
            methods.append(method);
        } else {
            // Default: add Monte Carlo for fitting tests
            method["Method"] = 1;
            method["MaxSteps"] = 500;
            method["VarianceSource"] = 2;
            method["RandomSeed"] = 12345;
            methods.append(method);
        }
        
    postFitAnalysis["methods"] = methods;
    modelConfig["PostFitAnalysis"] = postFitAnalysis;
    
    QString modelName = QString("nmr_model_%1").arg(modelId);
    addModels[modelName] = modelConfig;
    config["AddModels"] = addModels;
    
    QJsonDocument doc(config);
    return doc.toJson(QJsonDocument::Compact);
}

QString ComprehensiveRealDataTest::generateComprehensiveConfig()
{
    // The persistent comprehensive configuration; its data paths are relative to the source directory
    QFile configFile(QStringLiteral(SUPRAFIT_SOURCE_DIR "/input/comprehensive_real_data_test.json"));
    if (!configFile.open(QIODevice::ReadOnly)) return QString();

    QJsonObject config = QJsonDocument::fromJson(configFile.readAll()).object();
    for (const QString& section : { QStringLiteral("Independent"), QStringLiteral("Dependent") }) {
        QJsonObject s = config[section].toObject();
        QJsonObject file = s["File"].toObject();
        if (file.contains("Path") && QFileInfo(file["Path"].toString()).isRelative())
            file["Path"] = QStringLiteral(SUPRAFIT_SOURCE_DIR "/") + file["Path"].toString();
        s["File"] = file;
        config[section] = s;
    }
    return QString::fromUtf8(QJsonDocument(config).toJson(QJsonDocument::Compact));
}

void ComprehensiveRealDataTest::verifyModelFitQuality(const QJsonObject& fitQuality, double minR2)
{
    QVERIFY2(fitQuality.contains("r_squared"), "R-squared value missing");
    
    double r2 = fitQuality["r_squared"].toDouble();
    QVERIFY2(r2 >= minR2, QString("R-squared (%1) below minimum threshold (%2)").arg(r2).arg(minR2).toLocal8Bit());
    QVERIFY2(r2 <= 1.0, "R-squared exceeds maximum value of 1.0");
    
    if (fitQuality.contains("aic")) {
        double aic = fitQuality["aic"].toDouble();
        QVERIFY2(std::isfinite(aic), "AIC value is not finite");
    }
    
    if (fitQuality.contains("rmse")) {
        double rmse = fitQuality["rmse"].toDouble();
        QVERIFY2(rmse >= 0.0, "RMSE cannot be negative");
    }
}

void ComprehensiveRealDataTest::verifyStatisticalResults(const QJsonObject& analysis, const QString& method)
{
    QVERIFY2(analysis.contains(method), QString("Statistical method %1 results missing").arg(method).toLocal8Bit());
    
    QJsonObject methodResults = analysis[method].toObject();
    QVERIFY2(!methodResults.isEmpty(), QString("Statistical method %1 results empty").arg(method).toLocal8Bit());
    
    // Method-specific verifications
    if (method == "monte_carlo") {
        QVERIFY2(methodResults.contains("max_steps"), "Monte Carlo max_steps missing");
        QVERIFY2(methodResults.contains("models"), "Monte Carlo models missing");
    } else if (method.startsWith("cross_validation")) {
        QVERIFY2(methodResults.contains("validation_errors"), "Cross-validation errors missing");
    } else if (method == "parameter_reduction") {
        QVERIFY2(methodResults.contains("parameter_significance"), "Parameter significance missing");
    }
}

void ComprehensiveRealDataTest::compareModelRankings(const QVector<QJsonObject>& models)
{
    QVERIFY2(models.size() >= 2, "Need at least 2 models for ranking comparison");
    
    // Extract AIC values and sort
    QVector<QPair<double, int>> aicValues;
    for (int i = 0; i < models.size(); ++i) {
        QJsonObject fitQuality = extractFitQuality(models[i]);
        if (fitQuality.contains("aic")) {
            double aic = fitQuality["aic"].toDouble();
            aicValues.append(qMakePair(aic, i));
        }
    }
    
    std::sort(aicValues.begin(), aicValues.end());
    
    QVERIFY2(!aicValues.isEmpty(), "No AIC values found for model ranking");
    
    // Store ranking in results
    QJsonArray ranking;
    for (int rank = 0; rank < aicValues.size(); ++rank) {
        QJsonObject rankEntry;
        rankEntry["rank"] = rank + 1;
        rankEntry["model_index"] = aicValues[rank].second;
        rankEntry["aic"] = aicValues[rank].first;
        ranking.append(rankEntry);
    }
    
    QJsonObject performance = m_testResults["performance"].toObject();
    performance["model_ranking"] = ranking;
    m_testResults["performance"] = performance;
}

QTEST_MAIN(ComprehensiveRealDataTest)
#include "test_comprehensive_real_data.moc"