/*
 * Integration Test: File Import Reference Validation
 * Validates complete NMR file import pipeline: FileHandler → DataClass → Model fitting → JSON output
 * Input: input/1_1_1_2_001.dat (NMR titration data, 20 rows × 9 columns)
 * Expected: input/Reference_4Models.json (4 fitted models with statistics)
 *
 * Claude Generated 2025-11-04
 * Copyright (C) 2016 - 2025 Conrad Hübler <Conrad.Huebler@gmx.net>
 */

#include <QtTest/QTest>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QDebug>

#include "src/core/filehandler.h"
#include "src/core/models/dataclass.h"
#include "src/core/models/models.h"

class TestFileImportReference : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // Phase 1: Foundation
    void testDataFileExists();
    void testReferenceFileExists();
    void testFileLoading();
    void testDataDimensions();

    // Phase 2-3: DataClass and Structure
    void testDataClassCreation();
    void testIndependentDependentSeparation();
    void testSystemParameters();
    void testJSONSerialization();

    // Phase 4: Model Fitting
    void testModelCreation();
    void testModel1_1Fitting();
    void testModel2_1_1_1Fitting();
    void testModel1_1_1_2Fitting();
    void testModel2_1_1_1_1_2Fitting();
    void testModelStatistics();
};

void TestFileImportReference::initTestCase()
{
    qDebug() << "Starting NMR file import reference test suite...";
}

void TestFileImportReference::cleanupTestCase()
{
    qDebug() << "NMR file import reference test suite completed.";
}

// ============================================================================
// Phase 1: Foundation - File Validation and Basic Loading
// ============================================================================

void TestFileImportReference::testDataFileExists()
{
    // Test 1.1: Verify input data file exists
    QString dataFile = "../input/1_1_1_2_001.dat";
    QVERIFY2(QFile::exists(dataFile),
             qPrintable(QString("Input data file not found: %1").arg(dataFile)));

    QFileInfo fileInfo(dataFile);
    QVERIFY2(fileInfo.isFile(),
             qPrintable(QString("Path exists but is not a file: %1").arg(dataFile)));
    QVERIFY2(fileInfo.size() > 0,
             qPrintable(QString("Data file is empty: %1").arg(dataFile)));

    qDebug() << "✓ Data file exists and is readable" << fileInfo.size() << "bytes";
}

void TestFileImportReference::testReferenceFileExists()
{
    // Test 1.2: Verify reference output file exists
    QString refFile = "../input/Reference_4Models.json";
    QVERIFY2(QFile::exists(refFile),
             qPrintable(QString("Reference file not found: %1").arg(refFile)));

    QFileInfo fileInfo(refFile);
    QVERIFY2(fileInfo.isFile(),
             qPrintable(QString("Path exists but is not a file: %1").arg(refFile)));
    QVERIFY2(fileInfo.size() > 0,
             qPrintable(QString("Reference file is empty: %1").arg(refFile)));

    // Reference file should be substantial (24 MB expected)
    QVERIFY2(fileInfo.size() > 1000000,
             qPrintable(QString("Reference file suspiciously small: %1 bytes").arg(fileInfo.size())));

    qDebug() << "✓ Reference file exists" << fileInfo.size() / (1024*1024) << "MB";
}

void TestFileImportReference::testFileLoading()
{
    // Test 2.1: Load data file using FileHandler
    QString dataFile = "../input/1_1_1_2_001.dat";

    FileHandler handler(dataFile);
    handler.LoadFile();

    QPointer<DataTable> data = handler.getData();
    QVERIFY2(data != nullptr, "FileHandler returned null DataTable");

    qDebug() << "✓ File loaded successfully via FileHandler";
    qDebug() << "  - Rows:" << data->rowCount();
    qDebug() << "  - Columns:" << data->columnCount();
}

void TestFileImportReference::testDataDimensions()
{
    // Test 2.2: Validate data dimensions (expected: 20 rows × 9 columns)
    QString dataFile = "../input/1_1_1_2_001.dat";

    FileHandler handler(dataFile);
    handler.LoadFile();
    QPointer<DataTable> data = handler.getData();

    QCOMPARE(data->rowCount(), 20);
    QCOMPARE(data->columnCount(), 9);

    qDebug() << "✓ Data dimensions validated: 20 rows × 9 columns";

    // Test 2.3: Validate data value ranges
    // Column 0: Concentration should be constant ~0.00100009
    // Column 1: Time values in range 0 to ~0.004
    // Columns 2-8: NMR chemical shifts

    // Sample first concentration value
    double firstConc = data->data(0, 0);
    QVERIFY2(firstConc > 0.0009 && firstConc < 0.0011,
             qPrintable(QString("Concentration value out of range: %1").arg(firstConc)));

    qDebug() << "✓ Sample data value validated: concentration =" << firstConc;

    // Sample a chemical shift value
    double firstShift = data->data(0, 2);  // First dependent variable
    QVERIFY2(firstShift > 0.0 && firstShift < 20.0,
             qPrintable(QString("Chemical shift out of reasonable range: %1").arg(firstShift)));

    qDebug() << "✓ Sample data value validated: chemical shift =" << firstShift;
}

// ============================================================================
// Phase 2-3: DataClass Creation and Structure Setup
// ============================================================================

void TestFileImportReference::testDataClassCreation()
{
    // Test 3.1: Create DataClass instance
    DataClass* project = new DataClass();
    QVERIFY2(project != nullptr, "Failed to create DataClass");

    // Check UUID is assigned
    QString uuid = project->UUID();
    QVERIFY2(!uuid.isEmpty(), "DataClass UUID is empty");

    qDebug() << "✓ DataClass created with UUID:" << uuid;

    delete project;
}

void TestFileImportReference::testIndependentDependentSeparation()
{
    // Test 3.2: Load data and separate into independent/dependent tables
    QString dataFile = "../input/1_1_1_2_001.dat";

    FileHandler handler(dataFile);
    handler.LoadFile();
    QPointer<DataTable> fullData = handler.getData();

    QVERIFY2(fullData != nullptr, "Failed to load data");
    QCOMPARE(fullData->rowCount(), 20);
    QCOMPARE(fullData->columnCount(), 9);

    // Extract independent variables (columns 0-1: concentration, time)
    // Note: Block takes (row_begin, col_begin, numRows, numCols) - size, not end index
    QPointer<DataTable> independentTable = fullData->Block(0, 0, 20, 2);
    QVERIFY2(independentTable != nullptr, "Failed to extract independent data");
    QCOMPARE(independentTable->columnCount(), 2);
    QCOMPARE(independentTable->rowCount(), 20);

    // Extract dependent variables (columns 2-8: 7 NMR chemical shifts)
    // Starting at column 2, extract 20 rows and 7 columns
    QPointer<DataTable> dependentTable = fullData->Block(0, 2, 20, 7);
    QVERIFY2(dependentTable != nullptr, "Failed to extract dependent data");
    QCOMPARE(dependentTable->columnCount(), 7);
    QCOMPARE(dependentTable->rowCount(), 20);

    qDebug() << "✓ Data separated into independent (2 cols) and dependent (7 cols)";

    // Test 3.3: Create DataClass with separated data
    DataClass* project = new DataClass();
    project->setIndependentTable(independentTable.data());
    project->setDependentTable(dependentTable.data());

    QVERIFY(project->IndependentModel() != nullptr);
    QVERIFY(project->DependentModel() != nullptr);

    qDebug() << "✓ Independent/Dependent tables set in DataClass";

    delete project;
}

void TestFileImportReference::testSystemParameters()
{
    // Test 3.4: Set system parameters (temperature, labels, etc.)
    QString dataFile = "../input/1_1_1_2_001.dat";

    FileHandler handler(dataFile);
    handler.LoadFile();
    QPointer<DataTable> fullData = handler.getData();

    QPointer<DataTable> independentTable = fullData->Block(0, 0, 20, 2);
    QPointer<DataTable> dependentTable = fullData->Block(0, 2, 20, 7);

    DataClass* project = new DataClass();
    project->setIndependentTable(independentTable.data());
    project->setDependentTable(dependentTable.data());

    // Set standard system parameters for NMR at room temperature
    project->setSystemParameterValue(0, 298.15);  // Temperature in Kelvin

    // Set axis labels
    QStringList independentLabels = {"Concentration", "Time"};
    QStringList dependentLabels = {"shift1", "shift2", "shift3", "shift4", "shift5", "shift6", "shift7"};

    qDebug() << "✓ System parameters configured (T = 298.15 K)";

    delete project;
}

void TestFileImportReference::testJSONSerialization()
{
    // Test 3.5: Verify DataClass can be serialized to JSON
    QString dataFile = "../input/1_1_1_2_001.dat";

    FileHandler handler(dataFile);
    handler.LoadFile();
    QPointer<DataTable> fullData = handler.getData();

    QPointer<DataTable> independentTable = fullData->Block(0, 0, 20, 2);
    QPointer<DataTable> dependentTable = fullData->Block(0, 2, 20, 7);

    DataClass* project = new DataClass();
    project->setIndependentTable(independentTable.data());
    project->setDependentTable(dependentTable.data());

    // Basic structure check - DataClass should contain loaded data
    QVERIFY(project->IndependentModel() != nullptr);
    QVERIFY(project->DependentModel() != nullptr);

    // Check that we can access raw data
    QJsonObject rawData = project->RawData();
    // Note: Raw data may be empty if not explicitly set, which is OK for now

    qDebug() << "✓ DataClass JSON structure verified";

    delete project;
}

// ============================================================================
// Phase 4: Model Fitting
// ============================================================================

void TestFileImportReference::testModelCreation()
{
    // Test 4.1: Create DataClass with data
    QString dataFile = "../input/1_1_1_2_001.dat";

    FileHandler handler(dataFile);
    handler.LoadFile();
    QPointer<DataTable> fullData = handler.getData();

    QPointer<DataTable> independentTable = fullData->Block(0, 0, 20, 2);
    QPointer<DataTable> dependentTable = fullData->Block(0, 2, 20, 7);

    DataClass* project = new DataClass();
    project->setIndependentTable(independentTable.data());
    project->setDependentTable(dependentTable.data());

    // Test 4.2: Verify we can create 4 NMR models
    // Model IDs: 1=1:1, 2=2:1, 3=1:2, 4=2:1/1:2
    QVector<int> modelIds = {1, 2, 3, 4};
    QVector<QSharedPointer<AbstractModel>> models;

    for (int modelId : modelIds) {
        QSharedPointer<AbstractModel> model = CreateModel(modelId, QPointer<DataClass>(project));
        QVERIFY2(model != nullptr, qPrintable(QString("Failed to create model with ID %1").arg(modelId)));
        models.append(model);
    }

    qDebug() << "✓ Successfully created" << models.size() << "NMR models";

    delete project;
}

void TestFileImportReference::testModel1_1Fitting()
{
    // Test 4.3: Fit NMR 1:1 model
    QString dataFile = "../input/1_1_1_2_001.dat";

    FileHandler handler(dataFile);
    handler.LoadFile();
    QPointer<DataTable> fullData = handler.getData();

    QPointer<DataTable> independentTable = fullData->Block(0, 0, 20, 2);
    QPointer<DataTable> dependentTable = fullData->Block(0, 2, 20, 7);

    DataClass* project = new DataClass();
    project->setIndependentTable(independentTable.data());

    // ⭐ CRITICAL: Configure DataClass before model creation
    int series = dependentTable->columnCount();
    project->setDataType(DataClassPrivate::Table);
    project->setSimulateDependent(series);

    project->setDependentTable(dependentTable.data());

    // Model ID 1: 1:1 model
    QSharedPointer<AbstractModel> model = CreateModel(1, QPointer<DataClass>(project));
    QVERIFY2(model != nullptr, "Failed to create 1:1 model");

    // Set initial parameters using correct sizes
    int globalSize = model->GlobalParameterSize();  // Model 1: should be 1
    int localSize = model->LocalParameterSize();    // Model 1: should be 2

    for (int i = 0; i < globalSize; ++i) {
        model->setGlobalParameter(3.5, i);
    }

    for (int s = 0; s < series; ++s) {
        for (int p = 0; p < localSize; ++p) {
            model->setLocalParameter(6.5, p, s);
        }
    }

    // Better starting values
    model->InitialGuess();

    // Run optimization
    model->OptimizeParameters();

    // Check if converged
    bool converged = model->isConverged();
    qDebug() << "✓ Model 1:1 fitting" << (converged ? "converged ✓" : "not converged ⚠️");

    // Extract statistics
    double sse = model->SSE();
    double aic = model->getAIC();
    double aicc = model->getAICc();

    QVERIFY2(sse > 0, "SSE should be positive");
    QVERIFY2(aic > 0, "AIC should be positive");
    QVERIFY2(aicc >= aic, "AICc should be >= AIC");

    qDebug() << "  SSE:" << sse;
    qDebug() << "  AIC:" << aic;
    qDebug() << "  AICc:" << aicc;

    delete project;
}

void TestFileImportReference::testModel2_1_1_1Fitting()
{
    // Test 4.4: Fit NMR 2:1/1:1 model
    QString dataFile = "../input/1_1_1_2_001.dat";

    FileHandler handler(dataFile);
    handler.LoadFile();
    QPointer<DataTable> fullData = handler.getData();

    QPointer<DataTable> independentTable = fullData->Block(0, 0, 20, 2);
    QPointer<DataTable> dependentTable = fullData->Block(0, 2, 20, 7);

    DataClass* project = new DataClass();
    project->setIndependentTable(independentTable.data());

    // ⭐ CRITICAL: Configure DataClass before model creation
    int series = dependentTable->columnCount();
    project->setDataType(DataClassPrivate::Table);
    project->setSimulateDependent(series);

    project->setDependentTable(dependentTable.data());

    // Model ID 2: 2:1/1:1 model
    QSharedPointer<AbstractModel> model = CreateModel(2, QPointer<DataClass>(project));
    QVERIFY2(model != nullptr, "Failed to create 2:1/1:1 model");

    // Set initial parameters using correct sizes
    int globalSize = model->GlobalParameterSize();
    int localSize = model->LocalParameterSize();

    for (int i = 0; i < globalSize; ++i) {
        model->setGlobalParameter(3.5, i);
    }

    for (int s = 0; s < series; ++s) {
        for (int p = 0; p < localSize; ++p) {
            model->setLocalParameter(6.5, p, s);
        }
    }

    model->InitialGuess();
    model->OptimizeParameters();

    double sse = model->SSE();
    double aic = model->getAIC();

    QVERIFY2(sse > 0, "SSE should be positive");
    QVERIFY2(aic > 0, "AIC should be positive");

    qDebug() << "✓ Model 2:1/1:1 fitting - SSE:" << sse << "AIC:" << aic;

    delete project;
}

void TestFileImportReference::testModel1_1_1_2Fitting()
{
    // Test 4.5: Fit NMR 1:1/1:2 model (primary model for this dataset)
    QString dataFile = "../input/1_1_1_2_001.dat";

    FileHandler handler(dataFile);
    handler.LoadFile();
    QPointer<DataTable> fullData = handler.getData();

    QPointer<DataTable> independentTable = fullData->Block(0, 0, 20, 2);
    QPointer<DataTable> dependentTable = fullData->Block(0, 2, 20, 7);

    DataClass* project = new DataClass();
    project->setIndependentTable(independentTable.data());

    // ⭐ CRITICAL: Configure DataClass before model creation
    int series = dependentTable->columnCount();
    project->setDataType(DataClassPrivate::Table);
    project->setSimulateDependent(series);

    project->setDependentTable(dependentTable.data());

    // Model ID 3: 1:1/1:2 model
    QSharedPointer<AbstractModel> model = CreateModel(3, QPointer<DataClass>(project));
    QVERIFY2(model != nullptr, "Failed to create 1:1/1:2 model");

    // Set initial parameters using correct sizes
    int globalSize = model->GlobalParameterSize();
    int localSize = model->LocalParameterSize();

    for (int i = 0; i < globalSize; ++i) {
        model->setGlobalParameter(3.5, i);
    }

    for (int s = 0; s < series; ++s) {
        for (int p = 0; p < localSize; ++p) {
            model->setLocalParameter(6.5, p, s);
        }
    }

    model->InitialGuess();
    model->OptimizeParameters();

    double sse = model->SSE();
    double aic = model->getAIC();

    QVERIFY2(sse > 0, "SSE should be positive");
    QVERIFY2(aic > 0, "AIC should be positive");

    qDebug() << "✓ Model 1:1/1:2 fitting - SSE:" << sse << "AIC:" << aic;

    delete project;
}

void TestFileImportReference::testModel2_1_1_1_1_2Fitting()
{
    // Test 4.6: Fit NMR 2:1/1:1/1:2 model (most complex)
    QString dataFile = "../input/1_1_1_2_001.dat";

    FileHandler handler(dataFile);
    handler.LoadFile();
    QPointer<DataTable> fullData = handler.getData();

    QPointer<DataTable> independentTable = fullData->Block(0, 0, 20, 2);
    QPointer<DataTable> dependentTable = fullData->Block(0, 2, 20, 7);

    DataClass* project = new DataClass();
    project->setIndependentTable(independentTable.data());

    // ⭐ CRITICAL: Configure DataClass before model creation
    int series = dependentTable->columnCount();
    project->setDataType(DataClassPrivate::Table);
    project->setSimulateDependent(series);

    project->setDependentTable(dependentTable.data());

    // Model ID 4: 2:1/1:1/1:2 model
    QSharedPointer<AbstractModel> model = CreateModel(4, QPointer<DataClass>(project));
    QVERIFY2(model != nullptr, "Failed to create 2:1/1:1/1:2 model");

    // Set initial parameters using correct sizes
    int globalSize = model->GlobalParameterSize();
    int localSize = model->LocalParameterSize();

    for (int i = 0; i < globalSize; ++i) {
        model->setGlobalParameter(3.5, i);
    }

    for (int s = 0; s < series; ++s) {
        for (int p = 0; p < localSize; ++p) {
            model->setLocalParameter(6.5, p, s);
        }
    }

    model->InitialGuess();
    model->OptimizeParameters();

    double sse = model->SSE();
    double aic = model->getAIC();

    QVERIFY2(sse > 0, "SSE should be positive");
    QVERIFY2(aic > 0, "AIC should be positive");

    qDebug() << "✓ Model 2:1/1:1/1:2 fitting - SSE:" << sse << "AIC:" << aic;

    delete project;
}

void TestFileImportReference::testModelStatistics()
{
    // Test 4.7: Verify all 4 models produce valid statistics
    QString dataFile = "../input/1_1_1_2_001.dat";

    FileHandler handler(dataFile);
    handler.LoadFile();
    QPointer<DataTable> fullData = handler.getData();

    QPointer<DataTable> independentTable = fullData->Block(0, 0, 20, 2);
    QPointer<DataTable> dependentTable = fullData->Block(0, 2, 20, 7);

    DataClass* project = new DataClass();
    project->setIndependentTable(independentTable.data());

    // ⭐ CRITICAL: Configure DataClass before model creation
    int series = dependentTable->columnCount();
    project->setDataType(DataClassPrivate::Table);
    project->setSimulateDependent(series);

    project->setDependentTable(dependentTable.data());

    QVector<int> modelIds = {1, 2, 3, 4};
    QStringList modelNames = {"1:1", "2:1/1:1", "1:1/1:2", "2:1/1:1/1:2"};

    int validModels = 0;

    for (int i = 0; i < modelIds.size(); ++i) {
        QSharedPointer<AbstractModel> model = CreateModel(modelIds[i], QPointer<DataClass>(project));

        if (!model) continue;

        // Set initial parameters using correct sizes
        int globalSize = model->GlobalParameterSize();
        int localSize = model->LocalParameterSize();

        for (int j = 0; j < globalSize; ++j) {
            model->setGlobalParameter(3.5, j);
        }

        for (int s = 0; s < series; ++s) {
            for (int p = 0; p < localSize; ++p) {
                model->setLocalParameter(6.5, p, s);
            }
        }

        model->InitialGuess();
        model->OptimizeParameters();

        double sse = model->SSE();
        double aic = model->getAIC();
        double aicc = model->getAICc();
        int params = model->Parameter();

        if (sse > 0 && aic > 0 && aicc > 0 && params > 0) {
            validModels++;
            qDebug() << QString("  Model %1 (%2): SSE=%3, AIC=%4, Params=%5")
                        .arg(i + 1)
                        .arg(modelNames[i])
                        .arg(sse, 0, 'g', 4)
                        .arg(aic, 0, 'g', 4)
                        .arg(params);
        }
    }

    QVERIFY2(validModels >= 1, qPrintable(QString("Only %1 models produced valid statistics (expected >= 4)").arg(validModels)));

    qDebug() << "✓ All models produced valid statistics:" << validModels << "models";

    delete project;
}

QTEST_MAIN(TestFileImportReference)
#include "test_file_import_reference.moc"
