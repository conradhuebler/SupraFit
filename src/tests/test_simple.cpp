/*
 * Simple test to verify basic functionality
 * Copyright (C) 2024 Conrad Hübler <Conrad.Huebler@gmx.net>
 */

#include <QtTest/QTest>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QTemporaryFile>
#include <QtCore/QDebug>

class TestSimple : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testBasicJsonHandling();
    void testFileOperations();
    void testDataStructures();
};

void TestSimple::initTestCase()
{
    qDebug() << "Starting simple tests...";
}

void TestSimple::cleanupTestCase()
{
    qDebug() << "Simple tests completed.";
}

void TestSimple::testBasicJsonHandling()
{
    // Test basic JSON operations
    QJsonObject obj;
    obj["test"] = 42;
    obj["name"] = "test";
    
    QCOMPARE(obj["test"].toInt(), 42);
    QCOMPARE(obj["name"].toString(), QString("test"));
    
    // Test JSON array
    QJsonArray arr;
    arr.append(1.0);
    arr.append(2.0);
    arr.append(3.0);
    
    QCOMPARE(arr.size(), 3);
    QCOMPARE(arr[0].toDouble(), 1.0);
    QCOMPARE(arr[2].toDouble(), 3.0);
}

void TestSimple::testFileOperations()
{
    // Test file creation and reading
    QTemporaryFile file;
    QVERIFY(file.open());
    
    QString testData = "Hello World";
    file.write(testData.toUtf8());
    file.close();
    
    QVERIFY(QFileInfo::exists(file.fileName()));
    QVERIFY(QFileInfo(file.fileName()).size() > 0);
    
    // Test reading
    QFile readFile(file.fileName());
    QVERIFY(readFile.open(QIODevice::ReadOnly));
    
    QByteArray data = readFile.readAll();
    QString readData = QString::fromUtf8(data);
    
    QCOMPARE(readData, testData);
}

void TestSimple::testDataStructures()
{
    // Test Qt containers
    QList<int> intList;
    intList << 1 << 2 << 3 << 4 << 5;
    
    QCOMPARE(intList.size(), 5);
    QCOMPARE(intList[0], 1);
    QCOMPARE(intList[4], 5);
    
    // Test QStringList
    QStringList stringList;
    stringList << "alpha" << "beta" << "gamma";
    
    QCOMPARE(stringList.size(), 3);
    QCOMPARE(stringList[1], QString("beta"));
    
    // Test QHash
    QHash<QString, int> hash;
    hash["one"] = 1;
    hash["two"] = 2;
    hash["three"] = 3;
    
    QCOMPARE(hash.size(), 3);
    QCOMPARE(hash["two"], 2);
    QVERIFY(hash.contains("one"));
    QVERIFY(!hash.contains("four"));
}

QTEST_MAIN(TestSimple)
#include "test_simple.moc"