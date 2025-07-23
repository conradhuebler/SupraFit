/*
 * Unit tests for DataGenerator functionality
 * Copyright (C) 2025 Conrad Hübler <Conrad.Huebler@gmx.net>
 * Generated with Claude Code - Claude Code AI Assistant
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <QtTest/QtTest>
#include <QtCore/QJsonObject>
#include <QtCore/QPointer>

#include "src/capabilities/datagenerator.h"
#include "src/core/models/datatable.h"

class TestDataGenerator : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testLinearEquation();
    void testQuadraticEquation();
    void testMultipleIndependentVariables();
    void testInvalidEquations();
    void testRandomParameterGeneration();
    void testStaticRandomParameterGeneration(); // Claude Generated
    void testStaticRandomValueGeneration(); // Claude Generated
    void cleanupTestCase();

private:
    QPointer<DataGenerator> m_generator;
};

void TestDataGenerator::initTestCase()
{
    qDebug() << "Starting DataGenerator tests...";
    m_generator = new DataGenerator(this);
    QVERIFY(!m_generator.isNull());
}

void TestDataGenerator::testLinearEquation()
{
    // Test linear equation: y = 2*X + 1
    QJsonObject config;
    config["independent"] = 1;
    config["datapoints"] = 5;
    config["equations"] = "2*X + 1";
    
    m_generator->setJson(config);
    bool result = m_generator->Evaluate();
    
    QVERIFY(result);
    
    DataTable* table = m_generator->Table();
    QVERIFY(table != nullptr);
    QCOMPARE(table->rowCount(), 5);
    QCOMPARE(table->columnCount(), 1);
    
    // Check values: X=1: 2*1+1=3, X=2: 2*2+1=5, etc.
    QCOMPARE(table->data(0, 0), 3.0);  // X=1: 2*1+1=3
    QCOMPARE(table->data(1, 0), 5.0);  // X=2: 2*2+1=5  
    QCOMPARE(table->data(2, 0), 7.0);  // X=3: 2*3+1=7
    QCOMPARE(table->data(3, 0), 9.0);  // X=4: 2*4+1=9
    QCOMPARE(table->data(4, 0), 11.0); // X=5: 2*5+1=11
}

void TestDataGenerator::testQuadraticEquation()
{
    // Test quadratic equation: y = X^2
    QJsonObject config;
    config["independent"] = 1;
    config["datapoints"] = 4;
    config["equations"] = "X*X";
    
    m_generator->setJson(config);
    bool result = m_generator->Evaluate();
    
    QVERIFY(result);
    
    DataTable* table = m_generator->Table();
    QVERIFY(table != nullptr);
    QCOMPARE(table->rowCount(), 4);
    QCOMPARE(table->columnCount(), 1);
    
    // Check values: X=1: 1^2=1, X=2: 2^2=4, etc.
    QCOMPARE(table->data(0, 0), 1.0);  // X=1: 1^2=1
    QCOMPARE(table->data(1, 0), 4.0);  // X=2: 2^2=4
    QCOMPARE(table->data(2, 0), 9.0);  // X=3: 3^2=9
    QCOMPARE(table->data(3, 0), 16.0); // X=4: 4^2=16
}

void TestDataGenerator::testMultipleIndependentVariables()
{
    // Test multiple independent variables: x1 = X, x2 = X/2
    QJsonObject config;
    config["independent"] = 2;
    config["datapoints"] = 3;
    config["equations"] = "X|X/2";
    
    m_generator->setJson(config);
    bool result = m_generator->Evaluate();
    
    QVERIFY(result);
    
    DataTable* table = m_generator->Table();
    QVERIFY(table != nullptr);
    QCOMPARE(table->rowCount(), 3);
    QCOMPARE(table->columnCount(), 2);
    
    // Check first column (X)
    QCOMPARE(table->data(0, 0), 1.0);  // X=1
    QCOMPARE(table->data(1, 0), 2.0);  // X=2
    QCOMPARE(table->data(2, 0), 3.0);  // X=3
    
    // Check second column (X/2)
    QCOMPARE(table->data(0, 1), 0.5);  // X=1: 1/2=0.5
    QCOMPARE(table->data(1, 1), 1.0);  // X=2: 2/2=1.0
    QCOMPARE(table->data(2, 1), 1.5);  // X=3: 3/2=1.5
}

void TestDataGenerator::testInvalidEquations()
{
    // Test with mismatched number of equations and independent variables
    QJsonObject config;
    config["independent"] = 2;
    config["datapoints"] = 3;
    config["equations"] = "X";  // Only 1 equation for 2 independent variables
    
    m_generator->setJson(config);
    bool result = m_generator->Evaluate();
    
    QVERIFY(!result);  // Should fail due to mismatch
}

void TestDataGenerator::testRandomParameterGeneration()
{
    // Test enhanced random parameter generation - Claude Generated
    QJsonObject config;
    config["independent"] = 1;
    config["datapoints"] = 3;
    config["equations"] = "A * X + B";  // Parameters A and B will be random
    
    // Define random parameter limits
    QJsonObject randomLimits;
    QJsonObject paramA;
    paramA["min"] = 1.0;
    paramA["max"] = 5.0;
    randomLimits["A"] = paramA;
    
    QJsonObject paramB;
    paramB["min"] = 0.1;
    paramB["max"] = 1.0;
    randomLimits["B"] = paramB;
    
    m_generator->setJson(config);
    m_generator->setRandomSeed(12345); // Fixed seed for reproducible test
    bool result = m_generator->EvaluateWithRandomParameters(randomLimits);
    
    QVERIFY(result);
    
    DataTable* table = m_generator->Table();
    QVERIFY(table != nullptr);
    QCOMPARE(table->rowCount(), 3);
    QCOMPARE(table->columnCount(), 1);
    
    // Values should be within expected range based on parameter limits
    for (int i = 0; i < 3; ++i) {
        double value = table->data(i, 0);
        // X ranges from 1 to 3, A from 1 to 5, B from 0.1 to 1.0
        // Minimum: 1*1 + 0.1 = 1.1, Maximum: 5*3 + 1.0 = 16.0
        QVERIFY(value >= 1.1);
        QVERIFY(value <= 16.0);
    }
}

void TestDataGenerator::testStaticRandomParameterGeneration()
{
    // Test static random parameter generation method - Claude Generated
    QJsonObject limits;
    
    QJsonObject paramA;
    paramA["min"] = 2.0;
    paramA["max"] = 8.0;
    limits["A"] = paramA;
    
    QJsonObject paramB;
    paramB["min"] = -1.0;
    paramB["max"] = 1.0;
    limits["B"] = paramB;
    
    // Use fixed seed for reproducible test
    QJsonObject result = DataGenerator::generateRandomParameters(limits, 54321);
    
    QVERIFY(!result.isEmpty());
    QVERIFY(result.contains("A"));
    QVERIFY(result.contains("B"));
    
    // Check parameter A
    QJsonObject paramAResult = result["A"].toObject();
    double valueA = paramAResult["value"].toDouble();
    QVERIFY(valueA >= 2.0);
    QVERIFY(valueA <= 8.0);
    QCOMPARE(paramAResult["min"].toDouble(), 2.0);
    QCOMPARE(paramAResult["max"].toDouble(), 8.0);
    
    // Check parameter B
    QJsonObject paramBResult = result["B"].toObject();
    double valueB = paramBResult["value"].toDouble();
    QVERIFY(valueB >= -1.0);
    QVERIFY(valueB <= 1.0);
    QCOMPARE(paramBResult["min"].toDouble(), -1.0);
    QCOMPARE(paramBResult["max"].toDouble(), 1.0);
    
    // Generate again with same seed should give same results
    QJsonObject result2 = DataGenerator::generateRandomParameters(limits, 54321);
    QCOMPARE(result2["A"].toObject()["value"].toDouble(), valueA);
    QCOMPARE(result2["B"].toObject()["value"].toDouble(), valueB);
}

void TestDataGenerator::testStaticRandomValueGeneration()
{
    // Test static random value generation method - Claude Generated
    double min = 10.0;
    double max = 20.0;
    quint64 seed = 98765;
    
    // Generate random value
    double value1 = DataGenerator::generateRandomValue(min, max, seed);
    QVERIFY(value1 >= min);
    QVERIFY(value1 <= max);
    
    // Same seed should give same result
    double value2 = DataGenerator::generateRandomValue(min, max, seed);
    QCOMPARE(value1, value2);
    
    // Different seed should give different result (with high probability)
    double value3 = DataGenerator::generateRandomValue(min, max, seed + 1);
    QVERIFY(value3 >= min);
    QVERIFY(value3 <= max);
    // Note: Due to randomness, values could theoretically be the same, but very unlikely
}

void TestDataGenerator::cleanupTestCase()
{
    qDebug() << "DataGenerator tests completed.";
}

QTEST_MAIN(TestDataGenerator)
#include "test_datagenerator.moc"