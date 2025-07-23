/*
 * Comprehensive tests for DataClass (Project) loading and saving functionality
 * Copyright (C) 2024 Conrad Hübler <Conrad.Huebler@gmx.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <QtTest/QTest>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QTemporaryFile>
#include <QtCore/QDebug>

#include "src/core/models/dataclass.h"
#include "src/core/models/datatable.h"

class TestDataClass : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // Basic DataClass functionality
    void testCreateEmptyDataClass();
    void testCreateDataClassWithData();
    void testDataClassTypes();
    void testDataClassDimensions();

    // Table management
    void testIndependentTableSetting();
    void testDependentTableSetting();
    void testRawTableHandling();
    void testTableOverrides();

    // System Parameters
    void testSystemParameterAddition();
    void testSystemParameterRetrieval();
    void testSystemParameterTypes();
    void testSystemParameterPersistence();

    // JSON Export/Import
    void testProjectExport();
    void testProjectImport();
    void testProjectRoundTrip();
    void testExportWithStatistics();
    void testImportWithStatistics();

    // File I/O
    void testSaveProjectToFile();
    void testLoadProjectFromFile();
    void testProjectFileRoundTrip();
    void testInvalidProjectFileHandling();

    // UUID and metadata
    void testUUIDGeneration();
    void testProjectTitle();
    void testProjectContent();
    void testProjectMetadata();

    // Data range and filtering
    void testDataRangeSettings();
    void testDataFiltering();
    void testCheckedStateHandling();

    // Complex scenarios
    void testMultipleTableDataClass();
    void testLargeProjectHandling();
    void testProjectWithChildren();
    void testProjectCloning();

    // Error handling
    void testInvalidJsonImport();
    void testMissingDataHandling();
    void testCorruptedDataHandling();
    void testMemoryManagement();

private:
    DataClass* createTestDataClass();
    DataTable* createTestTable(int rows, int cols);
    void fillTestData(DataTable* table);
    QJsonObject createTestProjectJson();
    bool compareDataClasses(DataClass* dc1, DataClass* dc2);
    void addTestSystemParameters(DataClass* dc);
};

void TestDataClass::initTestCase()
{
    qDebug() << "Starting DataClass tests...";
}

void TestDataClass::cleanupTestCase()
{
    qDebug() << "DataClass tests completed.";
}

void TestDataClass::testCreateEmptyDataClass()
{
    DataClass* dc = new DataClass();
    QVERIFY(dc != nullptr);
    QVERIFY(!dc->UUID().isEmpty());
    QCOMPARE(dc->Type(), static_cast<int>(DataClassPrivate::DataType::Table));
    delete dc;
}

void TestDataClass::testCreateDataClassWithData()
{
    DataClass* dc = createTestDataClass();
    QVERIFY(dc != nullptr);
    QVERIFY(dc->IndependentModel() != nullptr);
    QVERIFY(dc->DependentModel() != nullptr);
    QVERIFY(dc->DataPoints() > 0);
    delete dc;
}

void TestDataClass::testDataClassTypes()
{
    DataClass* dc = new DataClass();
    
    // Test different data types
    dc->setType(DataClassPrivate::DataType::Table);
    QCOMPARE(dc->Type(), static_cast<int>(DataClassPrivate::DataType::Table));
    
    dc->setType(DataClassPrivate::DataType::Thermogram);
    QCOMPARE(dc->Type(), static_cast<int>(DataClassPrivate::DataType::Thermogram));
    
    dc->setType(DataClassPrivate::DataType::Spectrum);
    QCOMPARE(dc->Type(), static_cast<int>(DataClassPrivate::DataType::Spectrum));
    
    dc->setType(DataClassPrivate::DataType::Simulation);
    QCOMPARE(dc->Type(), static_cast<int>(DataClassPrivate::DataType::Simulation));
    QVERIFY(dc->isSimulation());
    
    delete dc;
}

void TestDataClass::testDataClassDimensions()
{
    DataClass* dc = createTestDataClass();
    
    QVERIFY(dc->DataPoints() > 0);
    QVERIFY(dc->IndependentVariableSize() > 0);
    QVERIFY(dc->SeriesCount() > 0);
    QCOMPARE(dc->Size(), dc->DataPoints());
    
    delete dc;
}

void TestDataClass::testIndependentTableSetting()
{
    DataClass* dc = new DataClass();
    DataTable* table = createTestTable(10, 2);
    fillTestData(table);
    
    dc->setIndependentTable(table);
    
    QCOMPARE(dc->IndependentModel(), table);
    QCOMPARE(dc->IndependentVariableSize(), 2);
    QCOMPARE(dc->DataPoints(), 10);
    
    delete dc;
}

void TestDataClass::testDependentTableSetting()
{
    DataClass* dc = new DataClass();
    DataTable* table = createTestTable(10, 3);
    fillTestData(table);
    
    dc->setDependentTable(table);
    
    QCOMPARE(dc->DependentModel(), table);
    QCOMPARE(dc->SeriesCount(), 3);
    QCOMPARE(dc->DataPoints(), 10);
    
    delete dc;
}

void TestDataClass::testRawTableHandling()
{
    DataClass* dc = new DataClass();
    DataTable* indepTable = createTestTable(5, 2);
    DataTable* depTable = createTestTable(5, 4);
    
    fillTestData(indepTable);
    fillTestData(depTable);
    
    dc->setIndependentRawTable(indepTable);
    dc->setDependentRawTable(depTable);
    
    QVERIFY(dc->IndependentRawModel() != nullptr);
    QVERIFY(dc->DependentRawModel() != nullptr);
    QCOMPARE(dc->IndependentRawModel()->rowCount(), 5);
    QCOMPARE(dc->DependentRawModel()->rowCount(), 5);
    
    delete dc;
}

void TestDataClass::testTableOverrides()
{
    DataClass* dc = createTestDataClass();
    DataTable* newTable = createTestTable(8, 5);
    fillTestData(newTable);
    
    dc->OverrideInDependentTable(newTable);
    QCOMPARE(dc->IndependentModel()->rowCount(), 8);
    QCOMPARE(dc->IndependentModel()->columnCount(), 5);
    
    DataTable* newDepTable = createTestTable(8, 3);
    fillTestData(newDepTable);
    
    dc->OverrideDependentTable(newDepTable);
    QCOMPARE(dc->DependentModel()->rowCount(), 8);
    QCOMPARE(dc->DependentModel()->columnCount(), 3);
    
    delete dc;
}

void TestDataClass::testSystemParameterAddition()
{
    DataClass* dc = new DataClass();
    
    // Add different types of system parameters
    dc->addSystemParameter(1, "TestString", "String parameter", SystemParameter::String);
    dc->addSystemParameter(2, "TestScalar", "Scalar parameter", SystemParameter::Scalar);
    dc->addSystemParameter(3, "TestBoolean", "Boolean parameter", SystemParameter::Boolean);
    dc->addSystemParameter(4, "TestList", "List parameter", SystemParameter::List);
    
    QList<int> paramList = dc->getSystemParameterList();
    QCOMPARE(paramList.size(), 4);
    QVERIFY(paramList.contains(1));
    QVERIFY(paramList.contains(2));
    QVERIFY(paramList.contains(3));
    QVERIFY(paramList.contains(4));
    
    delete dc;
}

void TestDataClass::testSystemParameterRetrieval()
{
    DataClass* dc = new DataClass();
    
    dc->addSystemParameter(1, "TestParam", "Test parameter", SystemParameter::Scalar);
    dc->setSystemParameterValue(1, 42.5);
    
    SystemParameter param = dc->getSystemParameter(1);
    QCOMPARE(param.Name(), QString("TestParam"));
    QCOMPARE(param.Description(), QString("Test parameter"));
    QCOMPARE(param.Double(), 42.5);
    QVERIFY(param.isScalar());
    
    delete dc;
}

void TestDataClass::testSystemParameterTypes()
{
    DataClass* dc = new DataClass();
    
    // Test String parameter
    dc->addSystemParameter(1, "StringParam", "String test", SystemParameter::String);
    dc->setSystemParameterValue(1, "Hello World");
    SystemParameter stringParam = dc->getSystemParameter(1);
    QCOMPARE(stringParam.getString(), QString("Hello World"));
    QVERIFY(stringParam.isString());
    
    // Test Boolean parameter
    dc->addSystemParameter(2, "BoolParam", "Boolean test", SystemParameter::Boolean);
    dc->setSystemParameterValue(2, true);
    SystemParameter boolParam = dc->getSystemParameter(2);
    QCOMPARE(boolParam.Bool(), true);
    QVERIFY(boolParam.isBool());
    
    // Test List parameter
    dc->addSystemParameter(3, "ListParam", "List test", SystemParameter::List);
    QStringList testList = {"Item1", "Item2", "Item3"};
    dc->setSystemParameterList(3, testList);
    SystemParameter listParam = dc->getSystemParameter(3);
    QCOMPARE(listParam.getList(), testList);
    QVERIFY(listParam.isList());
    
    delete dc;
}

void TestDataClass::testSystemParameterPersistence()
{
    DataClass* dc = new DataClass();
    addTestSystemParameters(dc);
    
    // Export and import
    QJsonObject exported = dc->ExportData();
    
    DataClass* newDc = new DataClass(exported);
    
    // Check that system parameters were preserved
    QList<int> paramList = newDc->getSystemParameterList();
    QVERIFY(paramList.size() > 0);
    
    delete dc;
    delete newDc;
}

void TestDataClass::testProjectExport()
{
    DataClass* dc = createTestDataClass();
    addTestSystemParameters(dc);
    
    QJsonObject exported = dc->ExportData();
    
    // Check basic structure
    QVERIFY(exported.contains("uuid"));
    QVERIFY(exported.contains("independent"));
    QVERIFY(exported.contains("dependent"));
    QVERIFY(exported.contains("DataType"));
    
    // Check that data is present
    QJsonObject independent = exported["independent"].toObject();
    QVERIFY(independent.contains("rows"));
    QVERIFY(independent.contains("cols"));
    QVERIFY(independent.contains("data"));
    
    delete dc;
}

void TestDataClass::testProjectImport()
{
    QJsonObject testProject = createTestProjectJson();
    
    DataClass* dc = new DataClass(testProject);
    
    QVERIFY(dc != nullptr);
    QCOMPARE(dc->DataPoints(), testProject["independent"].toObject()["rows"].toInt());
    QCOMPARE(dc->IndependentVariableSize(), testProject["independent"].toObject()["cols"].toInt());
    
    delete dc;
}

void TestDataClass::testProjectRoundTrip()
{
    DataClass* originalDc = createTestDataClass();
    addTestSystemParameters(originalDc);
    
    // Export
    QJsonObject exported = originalDc->ExportData();
    
    // Import
    DataClass* importedDc = new DataClass(exported);
    
    // Compare
    QCOMPARE(importedDc->DataPoints(), originalDc->DataPoints());
    QCOMPARE(importedDc->IndependentVariableSize(), originalDc->IndependentVariableSize());
    QCOMPARE(importedDc->SeriesCount(), originalDc->SeriesCount());
    QCOMPARE(importedDc->Type(), originalDc->Type());
    
    delete originalDc;
    delete importedDc;
}

void TestDataClass::testExportWithStatistics()
{
    DataClass* dc = createTestDataClass();
    
    QJsonObject exported = dc->ExportData();
    
    // Export with statistics should include additional metadata
    QVERIFY(exported.contains("uuid"));
    QVERIFY(exported.contains("DataType"));
    
    delete dc;
}

void TestDataClass::testImportWithStatistics()
{
    QJsonObject testProject = createTestProjectJson();
    
    // Add some statistics metadata
    testProject["statistics"] = QJsonObject();
    testProject["statistics"].toObject()["aic"] = 123.45;
    testProject["statistics"].toObject()["bic"] = 134.56;
    
    DataClass* dc = new DataClass(testProject);
    
    QVERIFY(dc != nullptr);
    // Statistics should be preserved in raw data
    QVERIFY(dc->RawData().contains("statistics"));
    
    delete dc;
}

void TestDataClass::testSaveProjectToFile()
{
    DataClass* dc = createTestDataClass();
    
    QTemporaryFile file;
    QVERIFY(file.open());
    
    QJsonObject exported = dc->ExportData();
    QJsonDocument doc(exported);
    
    file.write(doc.toJson());
    file.close();
    
    QVERIFY(QFileInfo::exists(file.fileName()));
    QVERIFY(QFileInfo(file.fileName()).size() > 0);
    
    delete dc;
}

void TestDataClass::testLoadProjectFromFile()
{
    // Create test project file
    QTemporaryFile file;
    QVERIFY(file.open());
    
    QJsonObject testProject = createTestProjectJson();
    QJsonDocument doc(testProject);
    file.write(doc.toJson());
    file.close();
    
    // Load from file using Qt's JSON handling
    QFile loadFile(file.fileName());
    QVERIFY(loadFile.open(QIODevice::ReadOnly));
    
    QByteArray data = loadFile.readAll();
    QJsonDocument loadDoc = QJsonDocument::fromJson(data);
    QJsonObject loadedProject = loadDoc.object();
    
    QVERIFY(!loadedProject.isEmpty());
    QVERIFY(loadedProject.contains("uuid"));
    QVERIFY(loadedProject.contains("independent"));
    QVERIFY(loadedProject.contains("dependent"));
}

void TestDataClass::testProjectFileRoundTrip()
{
    DataClass* originalDc = createTestDataClass();
    addTestSystemParameters(originalDc);
    
    // Save to file
    QTemporaryFile file;
    QVERIFY(file.open());
    
    QJsonObject exported = originalDc->ExportData();
    QJsonDocument doc(exported);
    file.write(doc.toJson());
    file.close();
    
    // Load from file using Qt's JSON handling
    QFile loadFile(file.fileName());
    QVERIFY(loadFile.open(QIODevice::ReadOnly));
    
    QByteArray data = loadFile.readAll();
    QJsonDocument loadDoc = QJsonDocument::fromJson(data);
    QJsonObject loadedProject = loadDoc.object();
    
    QVERIFY(!loadedProject.isEmpty());
    
    // Create new DataClass from loaded data
    DataClass* loadedDc = new DataClass(loadedProject);
    
    // Compare
    QCOMPARE(loadedDc->DataPoints(), originalDc->DataPoints());
    QCOMPARE(loadedDc->IndependentVariableSize(), originalDc->IndependentVariableSize());
    QCOMPARE(loadedDc->SeriesCount(), originalDc->SeriesCount());
    
    delete originalDc;
    delete loadedDc;
}

void TestDataClass::testInvalidProjectFileHandling()
{
    // Test creating DataClass from invalid JSON
    QJsonObject invalidData;
    invalidData["invalid"] = "structure";
    
    DataClass* dc = new DataClass(invalidData);
    // Should create an empty DataClass
    QVERIFY(dc != nullptr);
    
    delete dc;
}

void TestDataClass::testUUIDGeneration()
{
    DataClass* dc1 = new DataClass();
    DataClass* dc2 = new DataClass();
    
    QVERIFY(!dc1->UUID().isEmpty());
    QVERIFY(!dc2->UUID().isEmpty());
    QVERIFY(dc1->UUID() != dc2->UUID());
    
    delete dc1;
    delete dc2;
}

void TestDataClass::testProjectTitle()
{
    DataClass* dc = new DataClass();
    
    dc->setProjectTitle("Test Project");
    QCOMPARE(dc->ProjectTitle(), QString("Test Project"));
    
    delete dc;
}

void TestDataClass::testProjectContent()
{
    DataClass* dc = new DataClass();
    
    dc->setContent("Test content description");
    QCOMPARE(dc->Content(), QString("Test content description"));
    
    delete dc;
}

void TestDataClass::testProjectMetadata()
{
    DataClass* dc = new DataClass();
    
    dc->setProjectTitle("Test Project");
    dc->setContent("Test content");
    dc->setRootDir("/test/path");
    
    QJsonObject exported = dc->ExportData();
    
    DataClass* newDc = new DataClass(exported);
    
    // Metadata should be preserved
    QCOMPARE(newDc->ProjectTitle(), QString("Test Project"));
    QCOMPARE(newDc->Content(), QString("Test content"));
    QCOMPARE(newDc->RootDir(), QString("/test/path"));
    
    delete dc;
    delete newDc;
}

void TestDataClass::testDataRangeSettings()
{
    DataClass* dc = createTestDataClass();
    
    int totalPoints = dc->DataPoints();
    
    dc->setDataBegin(2);
    dc->setDataEnd(totalPoints - 2);
    
    QCOMPARE(dc->DataBegin(), 2);
    QCOMPARE(dc->DataEnd(), totalPoints - 2);
    
    delete dc;
}

void TestDataClass::testDataFiltering()
{
    DataClass* dc = createTestDataClass();
    
    // Test data range filtering
    dc->setDataBegin(1);
    dc->setDataEnd(5);
    
    QCOMPARE(dc->DataBegin(), 1);
    QCOMPARE(dc->DataEnd(), 5);
    
    delete dc;
}

void TestDataClass::testCheckedStateHandling()
{
    DataClass* dc = createTestDataClass();
    
    // Test checked state management
    dc->DependentModel()->setCheckable(true);
    dc->DependentModel()->CheckRow(0, true);
    dc->DependentModel()->CheckRow(1, false);
    dc->DependentModel()->CheckRow(2, true);
    
    QCOMPARE(dc->DependentModel()->isRowChecked(0), 1);
    QCOMPARE(dc->DependentModel()->isRowChecked(1), 0);
    QCOMPARE(dc->DependentModel()->isRowChecked(2), 1);
    
    delete dc;
}

void TestDataClass::testMultipleTableDataClass()
{
    DataClass* dc = new DataClass();
    
    // Set up multiple tables
    DataTable* indepTable = createTestTable(10, 2);
    DataTable* depTable = createTestTable(10, 5);
    DataTable* indepRawTable = createTestTable(10, 2);
    DataTable* depRawTable = createTestTable(10, 5);
    
    fillTestData(indepTable);
    fillTestData(depTable);
    fillTestData(indepRawTable);
    fillTestData(depRawTable);
    
    dc->setIndependentTable(indepTable);
    dc->setDependentTable(depTable);
    dc->setIndependentRawTable(indepRawTable);
    dc->setDependentRawTable(depRawTable);
    
    QCOMPARE(dc->IndependentModel()->rowCount(), 10);
    QCOMPARE(dc->DependentModel()->rowCount(), 10);
    QCOMPARE(dc->IndependentRawModel()->rowCount(), 10);
    QCOMPARE(dc->DependentRawModel()->rowCount(), 10);
    
    delete dc;
}

void TestDataClass::testLargeProjectHandling()
{
    DataClass* dc = new DataClass();
    
    // Create large tables
    DataTable* indepTable = createTestTable(1000, 10);
    DataTable* depTable = createTestTable(1000, 20);
    
    fillTestData(indepTable);
    fillTestData(depTable);
    
    dc->setIndependentTable(indepTable);
    dc->setDependentTable(depTable);
    
    // Add many system parameters
    for (int i = 0; i < 100; ++i) {
        dc->addSystemParameter(i, QString("Param%1").arg(i), 
                              QString("Description %1").arg(i), 
                              SystemParameter::Scalar);
        dc->setSystemParameterValue(i, i * 1.5);
    }
    
    // Test export/import
    QJsonObject exported = dc->ExportData();
    DataClass* newDc = new DataClass(exported);
    
    QCOMPARE(newDc->DataPoints(), 1000);
    QCOMPARE(newDc->IndependentVariableSize(), 10);
    QCOMPARE(newDc->SeriesCount(), 20);
    QCOMPARE(newDc->getSystemParameterList().size(), 100);
    
    delete dc;
    delete newDc;
}

void TestDataClass::testProjectWithChildren()
{
    DataClass* parent = createTestDataClass();
    
    // Note: Children functionality would need to be tested if implemented
    // For now, just test that children size is initially 0
    QCOMPARE(parent->ChildrenSize(), 0);
    
    delete parent;
}

void TestDataClass::testProjectCloning()
{
    DataClass* original = createTestDataClass();
    addTestSystemParameters(original);
    
    DataClass* clone = new DataClass(original);
    
    QCOMPARE(clone->DataPoints(), original->DataPoints());
    QCOMPARE(clone->IndependentVariableSize(), original->IndependentVariableSize());
    QCOMPARE(clone->SeriesCount(), original->SeriesCount());
    
    // UUID should be the same for clones
    QCOMPARE(clone->UUID(), original->UUID());
    
    delete original;
    delete clone;
}

void TestDataClass::testInvalidJsonImport()
{
    // Test various invalid JSON structures
    QJsonObject invalidJson1;
    invalidJson1["independent"] = "invalid";
    
    DataClass* dc1 = new DataClass(invalidJson1);
    QVERIFY(dc1 != nullptr); // Should create empty DataClass
    
    delete dc1;
    
    // Test missing required fields
    QJsonObject invalidJson2;
    invalidJson2["uuid"] = "test-uuid";
    // Missing tables
    
    DataClass* dc2 = new DataClass(invalidJson2);
    QVERIFY(dc2 != nullptr); // Should create empty DataClass
    
    delete dc2;
}

void TestDataClass::testMissingDataHandling()
{
    // Test DataClass with missing tables
    DataClass* dc = new DataClass();
    
    QCOMPARE(dc->DataPoints(), 0);
    QCOMPARE(dc->IndependentVariableSize(), 0);
    QCOMPARE(dc->SeriesCount(), 0);
    
    // Export should still work
    QJsonObject exported = dc->ExportData();
    QVERIFY(exported.contains("uuid"));
    
    delete dc;
}

void TestDataClass::testCorruptedDataHandling()
{
    // Test DataClass with corrupted table data
    QJsonObject corruptedProject = createTestProjectJson();
    
    // Corrupt the independent table data
    QJsonObject indepTable = corruptedProject["independent"].toObject();
    indepTable["rows"] = 5;
    indepTable["cols"] = 2;
    
    QJsonArray corruptedData;
    corruptedData.append("invalid"); // Non-numeric data
    corruptedData.append(1.5);
    indepTable["data"] = corruptedData;
    
    corruptedProject["independent"] = indepTable;
    
    // Should still create DataClass, but with limited functionality
    DataClass* dc = new DataClass(corruptedProject);
    QVERIFY(dc != nullptr);
    
    delete dc;
}

void TestDataClass::testMemoryManagement()
{
    // Test multiple DataClass creation/destruction
    for (int i = 0; i < 50; ++i) {
        DataClass* dc = createTestDataClass();
        addTestSystemParameters(dc);
        
        QJsonObject exported = dc->ExportData();
        DataClass* newDc = new DataClass(exported);
        
        delete dc;
        delete newDc;
    }
    
    // If we get here without crashes, memory management is working
    QVERIFY(true);
}

// Helper methods implementation
DataClass* TestDataClass::createTestDataClass()
{
    DataClass* dc = new DataClass();
    
    DataTable* indepTable = createTestTable(10, 2);
    DataTable* depTable = createTestTable(10, 3);
    
    fillTestData(indepTable);
    fillTestData(depTable);
    
    dc->setIndependentTable(indepTable);
    dc->setDependentTable(depTable);
    
    return dc;
}

DataTable* TestDataClass::createTestTable(int rows, int cols)
{
    return new DataTable(rows, cols, nullptr);
}

void TestDataClass::fillTestData(DataTable* table)
{
    for (int i = 0; i < table->rowCount(); ++i) {
        for (int j = 0; j < table->columnCount(); ++j) {
            table->data(i, j) = i * table->columnCount() + j + 0.5;
        }
    }
}

QJsonObject TestDataClass::createTestProjectJson()
{
    DataClass* dc = createTestDataClass();
    QJsonObject result = dc->ExportData();
    delete dc;
    return result;
}

bool TestDataClass::compareDataClasses(DataClass* dc1, DataClass* dc2)
{
    if (dc1->DataPoints() != dc2->DataPoints() ||
        dc1->IndependentVariableSize() != dc2->IndependentVariableSize() ||
        dc1->SeriesCount() != dc2->SeriesCount() ||
        dc1->Type() != dc2->Type()) {
        return false;
    }
    
    // Compare table data would require more detailed comparison
    // For now, just compare basic properties
    return true;
}

void TestDataClass::addTestSystemParameters(DataClass* dc)
{
    dc->addSystemParameter(1, "TestScalar", "Scalar test", SystemParameter::Scalar);
    dc->setSystemParameterValue(1, 42.5);
    
    dc->addSystemParameter(2, "TestString", "String test", SystemParameter::String);
    dc->setSystemParameterValue(2, "Test Value");
    
    dc->addSystemParameter(3, "TestBoolean", "Boolean test", SystemParameter::Boolean);
    dc->setSystemParameterValue(3, true);
    
    dc->addSystemParameter(4, "TestList", "List test", SystemParameter::List);
    dc->setSystemParameterList(4, QStringList() << "Item1" << "Item2" << "Item3");
}

QTEST_MAIN(TestDataClass)
#include "test_dataclass.moc"