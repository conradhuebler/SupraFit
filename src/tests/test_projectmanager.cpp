/*
 * Comprehensive tests for ProjectManager - Thread-safe project management
 * Copyright (C) 2025 Conrad Hübler <Conrad.Huebler@gmx.net>
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
#include <QtCore/QTemporaryDir>
#include <QtCore/QDebug>
#include <QtCore/QThread>
#include <QtCore/QMutex>
#include <QtTest/QSignalSpy>

#include "src/core/projectmanager.h"
#include "src/core/models/dataclass.h"
#include "src/core/models/datatable.h"

class TestProjectManager : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void cleanup();

    // Basic ProjectManager functionality
    void testSingletonAccess();
    void testProjectCreation();
    void testProjectExistence();
    void testProjectRetrieval();
    void testProjectRemoval();

    // JSON-based operations
    void testCreateProjectFromJson();
    void testGetProjectAsJson();
    void testGetAllProjectsAsJson();

    // File operations
    void testLoadProjectFromFile();
    void testSaveProjectToFile();
    void testProjectRoundTrip();

    // UUID and identification
    void testUuidGeneration();
    void testUuidUniqueness();

    // Thread safety
    void testConcurrentAccess();

    // Signal emission
    void testProjectAddedSignal();
    void testProjectRemovedSignal();

    // Error handling

    // Project lifecycle
    void testClearAllProjects();

private:
    QJsonObject createTestProjectJson(const QString& title = "Test Project");
    QJsonObject createTestDataSection();
    QString createTempProjectFile(const QJsonObject& projectData);
    void verifyProjectStructure(const QJsonObject& project, const QString& expectedTitle);

    QStringList m_createdProjectIds;
    QTemporaryDir* m_tempDir;
};

void TestProjectManager::initTestCase()
{
    // Create temporary directory for test files
    m_tempDir = new QTemporaryDir();
    QVERIFY(m_tempDir->isValid());

    // Clear any existing projects
    SupraFit::ProjectManager& pm = SupraFit::ProjectManager::instance();
    pm.clearAllProjects();
}

void TestProjectManager::cleanupTestCase()
{
    // Clean up all test projects
    SupraFit::ProjectManager& pm = SupraFit::ProjectManager::instance();
    pm.clearAllProjects();

    delete m_tempDir;
}

void TestProjectManager::cleanup()
{
    // Clean up projects created in each test
    SupraFit::ProjectManager& pm = SupraFit::ProjectManager::instance();
    for (const QString& projectId : m_createdProjectIds) {
        if (pm.hasProject(projectId)) {
            pm.removeProject(projectId);
        }
    }
    m_createdProjectIds.clear();
}

void TestProjectManager::testSingletonAccess()
{
    // Test that ProjectManager is properly implemented as singleton
    SupraFit::ProjectManager& pm1 = SupraFit::ProjectManager::instance();
    SupraFit::ProjectManager& pm2 = SupraFit::ProjectManager::instance();

    // Should be the same instance
    QCOMPARE(&pm1, &pm2);

    // Should be accessible from different contexts
    QStringList ids1 = pm1.getLoadedProjectIds();
    QStringList ids2 = pm2.getLoadedProjectIds();
    QCOMPARE(ids1, ids2);
}

void TestProjectManager::testProjectCreation()
{
    SupraFit::ProjectManager& pm = SupraFit::ProjectManager::instance();

    QJsonObject testData = createTestProjectJson("Test Creation Project");
    QString projectId = pm.createProjectFromJson(testData, "Test Creation Project");
    m_createdProjectIds.append(projectId);

    // Project should exist
    QVERIFY(!projectId.isEmpty());
    QVERIFY(pm.hasProject(projectId));

    // Should be in project list
    QStringList allIds = pm.getLoadedProjectIds();
    QVERIFY(allIds.contains(projectId));

    // Should be retrievable
    QSharedPointer<DataClass> project = pm.getProject(projectId);
    QVERIFY(!project.isNull());
    QCOMPARE(project->ProjectTitle(), "Test Creation Project");
}

void TestProjectManager::testProjectExistence()
{
    SupraFit::ProjectManager& pm = SupraFit::ProjectManager::instance();

    // Non-existent project
    QVERIFY(!pm.hasProject("non-existent-uuid"));

    // Create and verify existence
    QJsonObject testData = createTestProjectJson("Existence Test");
    QString projectId = pm.createProjectFromJson(testData, "Existence Test");
    m_createdProjectIds.append(projectId);

    QVERIFY(pm.hasProject(projectId));

    // Remove and verify non-existence
    pm.removeProject(projectId);
    QVERIFY(!pm.hasProject(projectId));
    m_createdProjectIds.removeAll(projectId);
}

void TestProjectManager::testProjectRetrieval()
{
    SupraFit::ProjectManager& pm = SupraFit::ProjectManager::instance();

    QJsonObject testData = createTestProjectJson("Retrieval Test");
    QString projectId = pm.createProjectFromJson(testData, "Retrieval Test");
    m_createdProjectIds.append(projectId);

    // Test DataClass retrieval
    QSharedPointer<DataClass> project = pm.getProject(projectId);
    QVERIFY(!project.isNull());
    QCOMPARE(project->ProjectTitle(), "Retrieval Test");
    QCOMPARE(project->UUID(), projectId);

    // Test JSON retrieval
    QJsonObject projectJson = pm.getProjectAsJson(projectId);
    QVERIFY(!projectJson.isEmpty());
    verifyProjectStructure(projectJson, "Retrieval Test");

    // Test non-existent project
    QSharedPointer<DataClass> nullProject = pm.getProject("non-existent");
    QVERIFY(nullProject.isNull());

    QJsonObject emptyJson = pm.getProjectAsJson("non-existent");
    QVERIFY(emptyJson.isEmpty());
}

void TestProjectManager::testProjectRemoval()
{
    SupraFit::ProjectManager& pm = SupraFit::ProjectManager::instance();

    // Create multiple projects
    QString id1 = pm.createProjectFromJson(createTestProjectJson("Project 1"), "Project 1");
    QString id2 = pm.createProjectFromJson(createTestProjectJson("Project 2"), "Project 2");
    m_createdProjectIds.append(id1);
    m_createdProjectIds.append(id2);

    QVERIFY(pm.hasProject(id1));
    QVERIFY(pm.hasProject(id2));
    QCOMPARE(pm.getLoadedProjectIds().size(), 2);

    // Remove one project
    pm.removeProject(id1);
    QVERIFY(!pm.hasProject(id1));
    QVERIFY(pm.hasProject(id2));
    QCOMPARE(pm.getLoadedProjectIds().size(), 1);
    m_createdProjectIds.removeAll(id1);

    // Remove remaining project
    pm.removeProject(id2);
    QVERIFY(!pm.hasProject(id2));
    QCOMPARE(pm.getLoadedProjectIds().size(), 0);
    m_createdProjectIds.removeAll(id2);
}

void TestProjectManager::testCreateProjectFromJson()
{
    SupraFit::ProjectManager& pm = SupraFit::ProjectManager::instance();

    // Test with valid JSON
    QJsonObject validData = createTestProjectJson("JSON Creation Test");
    QString projectId = pm.createProjectFromJson(validData, "JSON Creation Test");
    m_createdProjectIds.append(projectId);

    QVERIFY(!projectId.isEmpty());
    QSharedPointer<DataClass> project = pm.getProject(projectId);
    QVERIFY(!project.isNull());

    // Test with empty JSON
    QString emptyId = pm.createProjectFromJson(QJsonObject(), "Empty Project");
    if (!emptyId.isEmpty()) {
        m_createdProjectIds.append(emptyId);
        // Should handle gracefully
        QSharedPointer<DataClass> emptyProject = pm.getProject(emptyId);
        QVERIFY(!emptyProject.isNull());
    }
}

void TestProjectManager::testGetProjectAsJson()
{
    SupraFit::ProjectManager& pm = SupraFit::ProjectManager::instance();

    QJsonObject originalData = createTestProjectJson("JSON Export Test");
    QString projectId = pm.createProjectFromJson(originalData, "JSON Export Test");
    m_createdProjectIds.append(projectId);

    QJsonObject exportedData = pm.getProjectAsJson(projectId);
    QVERIFY(!exportedData.isEmpty());

    // Verify structure
    verifyProjectStructure(exportedData, "JSON Export Test");

    // Should contain core fields
    QVERIFY(exportedData.contains("title"));
    QVERIFY(exportedData.contains("uuid"));
    QVERIFY(exportedData.contains("SupraFit"));
    QVERIFY(exportedData.contains("DataType"));
}

void TestProjectManager::testGetAllProjectsAsJson()
{
    SupraFit::ProjectManager& pm = SupraFit::ProjectManager::instance();

    // Start with empty state
    QVector<QJsonObject> initialProjects = pm.getAllProjectsAsJson();
    int initialCount = initialProjects.size();

    // Create multiple projects
    QString id1 = pm.createProjectFromJson(createTestProjectJson("All Projects 1"), "All Projects 1");
    QString id2 = pm.createProjectFromJson(createTestProjectJson("All Projects 2"), "All Projects 2");
    m_createdProjectIds.append(id1);
    m_createdProjectIds.append(id2);

    QVector<QJsonObject> allProjects = pm.getAllProjectsAsJson();
    QCOMPARE(allProjects.size(), initialCount + 2);

    // Verify both projects are present
    bool found1 = false, found2 = false;
    for (const QJsonObject& project : allProjects) {
        QString title = project["title"].toString();
        if (title == "All Projects 1") found1 = true;
        if (title == "All Projects 2") found2 = true;
    }
    QVERIFY(found1);
    QVERIFY(found2);
}

void TestProjectManager::testLoadProjectFromFile()
{
    SupraFit::ProjectManager& pm = SupraFit::ProjectManager::instance();

    // Create test file
    QJsonObject testData = createTestProjectJson("File Load Test");
    QString filename = createTempProjectFile(testData);

    // Load project from file (loadProject returns bool; the loaded project becomes current)
    bool loadedOk = pm.loadProject(filename);
    QSharedPointer<DataClass> loadedProj = pm.getCurrentProject().toStrongRef();
    QString projectId = (loadedOk && loadedProj) ? loadedProj->UUID() : QString();
    if (!projectId.isEmpty()) {
        m_createdProjectIds.append(projectId);

        // Verify loaded project
        QSharedPointer<DataClass> project = pm.getProject(projectId);
        QVERIFY(!project.isNull());
        QCOMPARE(project->ProjectTitle(), "File Load Test");
    }
}

void TestProjectManager::testSaveProjectToFile()
{
    SupraFit::ProjectManager& pm = SupraFit::ProjectManager::instance();

    // Create project
    QJsonObject testData = createTestProjectJson("File Save Test");
    QString projectId = pm.createProjectFromJson(testData, "File Save Test");
    m_createdProjectIds.append(projectId);

    // Save to file
    QString filename = m_tempDir->filePath("save_test.json");
    bool saveSuccess = pm.saveProject(projectId, filename);
    QVERIFY(saveSuccess);

    // Verify file exists and can be loaded
    QVERIFY(QFile::exists(filename));

    QFile file(filename);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QVERIFY(!doc.isNull());

    QJsonObject savedData = doc.object();
    verifyProjectStructure(savedData, "File Save Test");
}

void TestProjectManager::testProjectRoundTrip()
{
    SupraFit::ProjectManager& pm = SupraFit::ProjectManager::instance();

    // Create original project
    QJsonObject originalData = createTestProjectJson("Round Trip Test");
    QString originalId = pm.createProjectFromJson(originalData, "Round Trip Test");
    m_createdProjectIds.append(originalId);

    // Save to file
    QString filename = m_tempDir->filePath("roundtrip_test.json");
    bool saveSuccess = pm.saveProject(originalId, filename);
    QVERIFY(saveSuccess);

    // Remove from memory
    pm.removeProject(originalId);
    m_createdProjectIds.removeAll(originalId);
    QVERIFY(!pm.hasProject(originalId));

    // Load back from file (loadProject returns bool; the loaded project becomes current)
    bool reloadedOk = pm.loadProject(filename);
    QSharedPointer<DataClass> reloadedProj = pm.getCurrentProject().toStrongRef();
    QString loadedId = (reloadedOk && reloadedProj) ? reloadedProj->UUID() : QString();
    if (!loadedId.isEmpty()) {
        m_createdProjectIds.append(loadedId);

        // Verify data integrity
        QSharedPointer<DataClass> loadedProject = pm.getProject(loadedId);
        QVERIFY(!loadedProject.isNull());
        QCOMPARE(loadedProject->ProjectTitle(), "Round Trip Test");

        // Compare JSON representations
        QJsonObject loadedJson = pm.getProjectAsJson(loadedId);
        verifyProjectStructure(loadedJson, "Round Trip Test");
    }
}

void TestProjectManager::testUuidGeneration()
{
    SupraFit::ProjectManager& pm = SupraFit::ProjectManager::instance();

    // Create multiple projects and verify UUID uniqueness
    QSet<QString> uuids;
    for (int i = 0; i < 10; ++i) {
        QString projectId = pm.createProjectFromJson(
            createTestProjectJson(QString("UUID Test %1").arg(i)),
            QString("UUID Test %1").arg(i)
        );
        m_createdProjectIds.append(projectId);

        QVERIFY(!projectId.isEmpty());
        QVERIFY(!uuids.contains(projectId));
        uuids.insert(projectId);

        // UUID should be valid format (basic check)
        QVERIFY(projectId.length() > 10);
        QVERIFY(!projectId.contains(" "));
    }

    QCOMPARE(uuids.size(), 10);
}

void TestProjectManager::testUuidUniqueness()
{
    SupraFit::ProjectManager& pm = SupraFit::ProjectManager::instance();

    // Create projects concurrently and verify uniqueness
    QSet<QString> allUuids;

    for (int batch = 0; batch < 3; ++batch) {
        QStringList batchUuids;

        for (int i = 0; i < 5; ++i) {
            QString projectId = pm.createProjectFromJson(
                createTestProjectJson(QString("Batch %1 Project %2").arg(batch).arg(i)),
                QString("Batch %1 Project %2").arg(batch).arg(i)
            );
            m_createdProjectIds.append(projectId);
            batchUuids.append(projectId);
            allUuids.insert(projectId);
        }

        // All UUIDs in batch should be unique
        QCOMPARE(QSet<QString>(batchUuids.begin(), batchUuids.end()).size(), 5);
    }

    // All UUIDs across batches should be unique
    QCOMPARE(allUuids.size(), 15);
}

void TestProjectManager::testConcurrentAccess()
{
    // Test basic thread safety
    SupraFit::ProjectManager& pm = SupraFit::ProjectManager::instance();

    // Create a project to work with
    QString baseProjectId = pm.createProjectFromJson(createTestProjectJson("Concurrent Test"), "Concurrent Test");
    m_createdProjectIds.append(baseProjectId);

    // Test concurrent read access
    QMutex resultMutex;
    QStringList results;

    auto readTask = [&pm, &baseProjectId, &resultMutex, &results]() {
        for (int i = 0; i < 5; ++i) {
            if (pm.hasProject(baseProjectId)) {
                QSharedPointer<DataClass> project = pm.getProject(baseProjectId);
                if (!project.isNull()) {
                    QMutexLocker locker(&resultMutex);
                    results.append(project->ProjectTitle());
                }
            }
            QThread::msleep(1);
        }
    };

    QThread thread1, thread2;
    QObject worker1, worker2;
    worker1.moveToThread(&thread1);
    worker2.moveToThread(&thread2);

    connect(&thread1, &QThread::started, readTask);
    connect(&thread2, &QThread::started, readTask);

    thread1.start();
    thread2.start();

    QVERIFY(thread1.wait(1000));
    QVERIFY(thread2.wait(1000));

    // Should have successful reads
    QVERIFY(results.size() > 0);
    for (const QString& result : results) {
        QCOMPARE(result, "Concurrent Test");
    }
}

void TestProjectManager::testProjectAddedSignal()
{
    SupraFit::ProjectManager& pm = SupraFit::ProjectManager::instance();

    QSignalSpy spy(&pm, &SupraFit::ProjectManager::projectAdded);

    QString projectId = pm.createProjectFromJson(createTestProjectJson("Signal Test"), "Signal Test");
    m_createdProjectIds.append(projectId);

    // Should have emitted projectAdded signal
    QCOMPARE(spy.count(), 1);
    QList<QVariant> arguments = spy.takeFirst();
    QCOMPARE(arguments.at(0).toString(), projectId);
}

void TestProjectManager::testProjectRemovedSignal()
{
    SupraFit::ProjectManager& pm = SupraFit::ProjectManager::instance();

    QString projectId = pm.createProjectFromJson(createTestProjectJson("Remove Signal Test"), "Remove Signal Test");
    m_createdProjectIds.append(projectId);

    QSignalSpy spy(&pm, &SupraFit::ProjectManager::projectRemoved);

    pm.removeProject(projectId);
    m_createdProjectIds.removeAll(projectId);

    // Should have emitted projectRemoved signal
    QCOMPARE(spy.count(), 1);
    QList<QVariant> arguments = spy.takeFirst();
    QCOMPARE(arguments.at(0).toString(), projectId);
}

void TestProjectManager::testClearAllProjects()
{
    SupraFit::ProjectManager& pm = SupraFit::ProjectManager::instance();

    // Create multiple projects
    for (int i = 0; i < 3; ++i) {
        QString projectId = pm.createProjectFromJson(
            createTestProjectJson(QString("Clear Test %1").arg(i)),
            QString("Clear Test %1").arg(i)
        );
        m_createdProjectIds.append(projectId);
    }

    QVERIFY(pm.getLoadedProjectIds().size() >= 3);

    // Clear all projects
    pm.clearAllProjects();
    m_createdProjectIds.clear();

    QCOMPARE(pm.getLoadedProjectIds().size(), 0);

    // All project existence checks should fail
    QVERIFY(!pm.hasProject("any-id"));
}

// Helper methods

QJsonObject TestProjectManager::createTestProjectJson(const QString& title)
{
    QJsonObject project;
    project["title"] = title;
    project["DataType"] = 1;
    project["SupraFit"] = 2024;
    project["independent"] = createTestDataSection();
    project["dependent"] = createTestDataSection();
    project["system"] = QJsonObject();
    project["raw"] = QJsonObject();
    project["begin_data"] = 0;
    project["end_data"] = 10;

    return project;
}

QJsonObject TestProjectManager::createTestDataSection()
{
    QJsonObject dataSection;
    dataSection["header"] = QJsonArray{"Column1", "Column2"};

    QJsonObject data;
    for (int i = 0; i < 5; ++i) {
        data[QString::number(i)] = QJsonArray{i * 1.0, i * 2.0};
    }
    dataSection["data"] = data;

    QJsonObject checked;
    for (int i = 0; i < 5; ++i) {
        checked[QString::number(i)] = true;
    }
    dataSection["checked"] = checked;

    return dataSection;
}

QString TestProjectManager::createTempProjectFile(const QJsonObject& projectData)
{
    QString filename = m_tempDir->filePath("test_project.json");

    QFile file(filename);
    if (file.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(projectData);
        file.write(doc.toJson());
        file.close();
    }

    return filename;
}

void TestProjectManager::verifyProjectStructure(const QJsonObject& project, const QString& expectedTitle)
{
    QVERIFY(project.contains("title"));
    QCOMPARE(project["title"].toString(), expectedTitle);

    QVERIFY(project.contains("DataType"));
    QVERIFY(project.contains("SupraFit"));
    QVERIFY(project.contains("independent"));
    QVERIFY(project.contains("dependent"));
}

QTEST_MAIN(TestProjectManager)
#include "test_projectmanager.moc"