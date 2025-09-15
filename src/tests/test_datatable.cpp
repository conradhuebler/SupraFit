/*
 * Comprehensive tests for DataTable loading and saving functionality
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

#include "src/core/models/datatable.h"

class TestDataTable : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // Basic DataTable functionality
    void testCreateEmptyTable();
    void testCreateTableWithData();
    void testTableDimensions();
    void testDataAccess();
    void testDataModification();

    // JSON Export/Import Tests
    void testJsonExport();
    void testJsonImport();
    void testJsonRoundTrip();
    void testJsonExportWithCheckedData();
    void testJsonImportWithCheckedData();

    // File I/O Tests
    void testSaveToFile();
    void testLoadFromFile();
    void testFileRoundTrip();
    void testInvalidFileHandling();

    // Header functionality
    void testHeaderSetting();
    void testHeaderExportImport();

    // Checked data functionality
    void testCheckedDataHandling();
    void testRowChecking();
    void testColumnChecking();

    // Edge cases and error handling
    void testEmptyTableHandling();
    void testLargeTableHandling();
    void testInvalidJsonHandling();
    void testMemoryManagement();

private:
    DataTable* createTestTable(int rows, int cols);
    void fillTestData(DataTable* table);
    bool compareDataTables(DataTable* table1, DataTable* table2);
    QJsonObject createTestJsonData();
};

void TestDataTable::initTestCase()
{
    qDebug() << "Starting DataTable tests...";
}

void TestDataTable::cleanupTestCase()
{
    qDebug() << "DataTable tests completed.";
}

void TestDataTable::testCreateEmptyTable()
{
    DataTable* table = new DataTable();
    QVERIFY(table != nullptr);
    QCOMPARE(table->rowCount(), 0);
    QCOMPARE(table->columnCount(), 0);
    delete table;
}

void TestDataTable::testCreateTableWithData()
{
    const int rows = 5, cols = 3;
    DataTable* table = createTestTable(rows, cols);
    
    QVERIFY(table != nullptr);
    QCOMPARE(table->rowCount(), rows);
    QCOMPARE(table->columnCount(), cols);
    
    delete table;
}

void TestDataTable::testTableDimensions()
{
    // Test various table dimensions
    QVector<QPair<int, int>> dimensions = {
        {1, 1}, {10, 5}, {100, 10}, {2, 20}, {0, 0}
    };
    
    for (const auto& dim : dimensions) {
        DataTable* table = new DataTable(dim.first, dim.second, nullptr);
        QCOMPARE(table->rowCount(), dim.first);
        QCOMPARE(table->columnCount(), dim.second);
        delete table;
    }
}

void TestDataTable::testDataAccess()
{
    const int rows = 3, cols = 4;
    DataTable* table = createTestTable(rows, cols);
    fillTestData(table);
    
    // Test data access
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            qreal expected = i * cols + j;
            QCOMPARE(table->data(i, j), expected);
        }
    }
    
    delete table;
}

void TestDataTable::testDataModification()
{
    DataTable* table = createTestTable(3, 3);
    
    // Test data modification
    table->data(1, 1) = 99.5;
    QCOMPARE(table->data(1, 1), 99.5);
    
    table->data(0, 2) = -123.456;
    QCOMPARE(table->data(0, 2), -123.456);
    
    delete table;
}

void TestDataTable::testJsonExport()
{
    DataTable* table = createTestTable(3, 2);
    fillTestData(table);
    
    QJsonObject jsonData = table->ExportTable(false);
    
    // Check basic structure
    QVERIFY(jsonData.contains("rows"));
    QVERIFY(jsonData.contains("cols"));
    QVERIFY(jsonData.contains("data"));
    
    QCOMPARE(jsonData["rows"].toInt(), 3);
    QCOMPARE(jsonData["cols"].toInt(), 2);
    
    // Check data object (actual API uses object with row keys, not array)
    QJsonObject dataObject = jsonData["data"].toObject();
    QCOMPARE(dataObject.size(), 3); // 3 rows
    
    delete table;
}

void TestDataTable::testJsonImport()
{
    QJsonObject testData = createTestJsonData();
    
    DataTable* table = new DataTable();
    bool success = table->ImportTable(testData);
    
    QVERIFY(success);
    QCOMPARE(table->rowCount(), testData["rows"].toInt());
    QCOMPARE(table->columnCount(), testData["cols"].toInt());
    
    delete table;
}

void TestDataTable::testJsonRoundTrip()
{
    DataTable* originalTable = createTestTable(4, 3);
    fillTestData(originalTable);
    
    // Export to JSON
    QJsonObject jsonData = originalTable->ExportTable(false);
    
    // Import from JSON
    DataTable* importedTable = new DataTable();
    bool success = importedTable->ImportTable(jsonData);
    
    QVERIFY(success);
    QVERIFY(compareDataTables(originalTable, importedTable));
    
    delete originalTable;
    delete importedTable;
}

void TestDataTable::testJsonExportWithCheckedData()
{
    DataTable* table = createTestTable(4, 2);
    fillTestData(table);
    
    // Set some rows as unchecked
    table->CheckRow(0, false);
    table->CheckRow(2, false);
    
    QJsonObject jsonData = table->ExportTable(true);
    
    QVERIFY(jsonData.contains("checked"));
    QJsonObject checkedObject = jsonData["checked"].toObject();
    QCOMPARE(checkedObject.size(), 4); // 4 rows
    
    delete table;
}

void TestDataTable::testJsonImportWithCheckedData()
{
    QJsonObject testData = createTestJsonData();
    
    // Add checked data in correct format: object with row keys
    QJsonObject checkedObject;
    checkedObject["0"] = "1 0 1";  // Row 0: checked, unchecked, checked
    checkedObject["1"] = "0 1 0";  // Row 1: unchecked, checked, unchecked  
    testData["checked"] = checkedObject;
    
    DataTable* table = new DataTable();
    bool success = table->ImportTable(testData);
    
    QVERIFY(success);
    QCOMPARE(table->isChecked(0, 0), true);   // Row 0, Col 0: 1 (checked)
    QCOMPARE(table->isChecked(0, 1), false);  // Row 0, Col 1: 0 (unchecked)
    QCOMPARE(table->isChecked(0, 2), true);   // Row 0, Col 2: 1 (checked)
    QCOMPARE(table->isChecked(1, 0), false);  // Row 1, Col 0: 0 (unchecked)
    QCOMPARE(table->isChecked(1, 1), true);   // Row 1, Col 1: 1 (checked)
    
    delete table;
}

void TestDataTable::testSaveToFile()
{
    DataTable* table = createTestTable(3, 2);
    fillTestData(table);
    
    QTemporaryFile file;
    QVERIFY(file.open());
    
    QJsonObject jsonData = table->ExportTable(false);
    QJsonDocument doc(jsonData);
    
    file.write(doc.toJson());
    file.close();
    
    QVERIFY(QFileInfo::exists(file.fileName()));
    QVERIFY(QFileInfo(file.fileName()).size() > 0);
    
    delete table;
}

void TestDataTable::testLoadFromFile()
{
    // Create test data file
    QTemporaryFile file;
    QVERIFY(file.open());
    
    QJsonObject testData = createTestJsonData();
    QJsonDocument doc(testData);
    file.write(doc.toJson());
    file.close();
    
    // Load from file using Qt's JSON handling
    QFile loadFile(file.fileName());
    QVERIFY(loadFile.open(QIODevice::ReadOnly));
    
    QByteArray data = loadFile.readAll();
    QJsonDocument loadDoc = QJsonDocument::fromJson(data);
    QJsonObject loadedData = loadDoc.object();
    
    QVERIFY(!loadedData.isEmpty());
    QCOMPARE(loadedData["rows"].toInt(), 2);
    QCOMPARE(loadedData["cols"].toInt(), 3);
}

void TestDataTable::testFileRoundTrip()
{
    DataTable* originalTable = createTestTable(5, 4);
    fillTestData(originalTable);
    
    // Save to file
    QTemporaryFile file;
    QVERIFY(file.open());
    
    QJsonObject jsonData = originalTable->ExportTable(false);
    QJsonDocument doc(jsonData);
    file.write(doc.toJson());
    file.close();
    
    // Load from file using Qt's JSON handling
    QFile loadFile(file.fileName());
    QVERIFY(loadFile.open(QIODevice::ReadOnly));
    
    QByteArray data = loadFile.readAll();
    QJsonDocument loadDoc = QJsonDocument::fromJson(data);
    QJsonObject loadedData = loadDoc.object();
    
    QVERIFY(!loadedData.isEmpty());
    
    // Create new table from loaded data
    DataTable* loadedTable = new DataTable();
    bool success = loadedTable->ImportTable(loadedData);
    QVERIFY(success);
    
    // Compare tables
    QVERIFY(compareDataTables(originalTable, loadedTable));
    
    delete originalTable;
    delete loadedTable;
}

void TestDataTable::testInvalidFileHandling()
{
    // Test importing incomplete JSON - API is permissive and defaults missing values
    DataTable* table = new DataTable();
    QJsonObject invalidData;
    invalidData["invalid"] = "data";
    
    bool success = table->ImportTable(invalidData);
    QVERIFY(success);  // API is permissive and always succeeds, defaulting missing values
    
    delete table;
}

void TestDataTable::testHeaderSetting()
{
    DataTable* table = createTestTable(3, 4);
    
    QStringList headers = {"Col1", "Col2", "Col3", "Col4"};
    table->setHeader(headers);
    
    QStringList retrievedHeaders = table->header();
    QCOMPARE(retrievedHeaders, headers);
    
    delete table;
}

void TestDataTable::testHeaderExportImport()
{
    DataTable* table = createTestTable(2, 3);
    QStringList headers = {"X", "Y", "Z"};
    table->setHeader(headers);
    
    QJsonObject jsonData = table->ExportTable(false);
    
    DataTable* newTable = new DataTable();
    bool success = newTable->ImportTable(jsonData);
    
    QVERIFY(success);
    QCOMPARE(newTable->header(), headers);
    
    delete table;
    delete newTable;
}

void TestDataTable::testCheckedDataHandling()
{
    DataTable* table = createTestTable(3, 2);
    table->setCheckable(true);
    
    // Test row checking
    table->CheckRow(0, true);
    table->CheckRow(1, false);
    table->CheckRow(2, true);
    
    QCOMPARE(table->isRowChecked(0), 1);  // Claude Generated - Fixed to expect 1 for checked row
    QCOMPARE(table->isRowChecked(1), 0);  // 0 columns checked = 0
    QCOMPARE(table->isRowChecked(2), 1);  // Claude Generated - Fixed to expect 1 for checked row
    
    delete table;
}

void TestDataTable::testRowChecking()
{
    DataTable* table = createTestTable(4, 3);
    table->setCheckable(true);
    
    // Check specific rows
    table->CheckRow(0);
    table->CheckRow(2);
    
    QCOMPARE(table->EnabledRows(), 2);  // 2 checked rows (updated for Claude's boolean isRowChecked fix)
    
    // Enable all rows
    table->EnableAllRows();
    QCOMPARE(table->EnabledRows(), 4); // 4 checked rows (updated for Claude's boolean isRowChecked fix)
    
    delete table;
}

void TestDataTable::testColumnChecking()
{
    DataTable* table = createTestTable(3, 3);
    table->setCheckable(true);
    
    // Test individual cell checking
    table->setChecked(0, 0, true);
    table->setChecked(0, 1, false);
    table->setChecked(1, 0, true);
    
    QCOMPARE(table->isChecked(0, 0), true);
    QCOMPARE(table->isChecked(0, 1), false);
    QCOMPARE(table->isChecked(1, 0), true);
    
    delete table;
}

void TestDataTable::testEmptyTableHandling()
{
    DataTable* table = new DataTable();
    
    // Test operations on empty table
    QCOMPARE(table->rowCount(), 0);
    QCOMPARE(table->columnCount(), 0);
    
    QJsonObject jsonData = table->ExportTable(false);
    QVERIFY(jsonData.contains("rows"));
    QVERIFY(jsonData.contains("cols"));
    QCOMPARE(jsonData["rows"].toInt(), 0);
    QCOMPARE(jsonData["cols"].toInt(), 0);
    
    delete table;
}

void TestDataTable::testLargeTableHandling()
{
    const int rows = 1000, cols = 50;
    DataTable* table = createTestTable(rows, cols);
    
    // Fill with test data
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            table->data(i, j) = i * cols + j;
        }
    }
    
    // Test dimensions
    QCOMPARE(table->rowCount(), rows);
    QCOMPARE(table->columnCount(), cols);
    
    // Test JSON export/import
    QJsonObject jsonData = table->ExportTable(false);
    
    DataTable* newTable = new DataTable();
    bool success = newTable->ImportTable(jsonData);
    
    QVERIFY(success);
    QCOMPARE(newTable->rowCount(), rows);
    QCOMPARE(newTable->columnCount(), cols);
    
    // Spot check some values
    QCOMPARE(newTable->data(0, 0), 0.0);
    QCOMPARE(newTable->data(999, 49), 999.0 * 50 + 49);
    
    delete table;
    delete newTable;
}

void TestDataTable::testInvalidJsonHandling()
{
    DataTable* table = new DataTable();
    
    // Test various edge-case JSON structures - API is permissive
    QJsonObject invalidJson1;
    invalidJson1["rows"] = -1;
    invalidJson1["cols"] = 5;
    QVERIFY(table->ImportTable(invalidJson1)); // API uses qMax() with current size, handles gracefully
    
    QJsonObject invalidJson2;
    invalidJson2["rows"] = 5;
    invalidJson2["cols"] = -1;
    QVERIFY(table->ImportTable(invalidJson2)); // API uses qMax() with current size, handles gracefully
    
    QJsonObject invalidJson3;
    invalidJson3["rows"] = 2;
    invalidJson3["cols"] = 2;
    invalidJson3["data"] = "invalid";
    QVERIFY(table->ImportTable(invalidJson3)); // API tries to convert and defaults on failure
    
    delete table;
}

void TestDataTable::testMemoryManagement()
{
    // Test multiple table creation/destruction
    for (int i = 0; i < 100; ++i) {
        DataTable* table = createTestTable(10, 10);
        fillTestData(table);
        
        QJsonObject jsonData = table->ExportTable(false);
        
        DataTable* newTable = new DataTable();
        newTable->ImportTable(jsonData);
        
        delete table;
        delete newTable;
    }
    
    // If we get here without crashes, memory management is working
    QVERIFY(true);
}

// Helper methods implementation
DataTable* TestDataTable::createTestTable(int rows, int cols)
{
    return new DataTable(rows, cols, nullptr);
}

void TestDataTable::fillTestData(DataTable* table)
{
    for (int i = 0; i < table->rowCount(); ++i) {
        for (int j = 0; j < table->columnCount(); ++j) {
            table->data(i, j) = i * table->columnCount() + j;
        }
    }
}

bool TestDataTable::compareDataTables(DataTable* table1, DataTable* table2)
{
    if (table1->rowCount() != table2->rowCount() || 
        table1->columnCount() != table2->columnCount()) {
        return false;
    }
    
    for (int i = 0; i < table1->rowCount(); ++i) {
        for (int j = 0; j < table1->columnCount(); ++j) {
            if (qAbs(table1->data(i, j) - table2->data(i, j)) > 1e-10) {
                return false;
            }
        }
    }
    
    return true;
}

QJsonObject TestDataTable::createTestJsonData()
{
    QJsonObject testData;
    testData["rows"] = 2;
    testData["cols"] = 3;
    
    // Use actual API format: data as object with row keys
    QJsonObject dataObject;
    dataObject["0"] = "0 1.5 3";       // Row 0: 0, 1.5, 3
    dataObject["1"] = "4.5 6 7.5";     // Row 1: 4.5, 6, 7.5
    testData["data"] = dataObject;
    
    return testData;
}

QTEST_MAIN(TestDataTable)
#include "test_datatable.moc"