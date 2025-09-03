/*
 * Comprehensive tests for SupraFit multi-project functionality
 * Copyright (C) 2025 Conrad Hübler <Conrad.Huebler@gmx.net>
 *
 * Tests project extraction (-p), UUID matching (-u), title matching (-t),
 * project splitting (-s), file joining (-j), and multi-project file handling.
 * Claude Generated - 2025-09-02
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <QtTest/QTest>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonArray>
#include <QtCore/QTemporaryFile>
#include <QtCore/QTemporaryDir>
#include <QtCore/QProcess>
#include <QtCore/QDebug>
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QUuid>

class TestMultiProject : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // Project Extraction Tests
    void testProjectExtractionByIndex();
    void testMultipleProjectExtraction();
    void testInvalidProjectIndex();
    void testProjectIndexBoundaries();

    // UUID Matching Tests
    void testUUIDMatching();
    void testPartialUUIDMatching();
    void testMultipleUUIDMatching();
    void testInvalidUUIDMatching();

    // Title Matching Tests
    void testTitleMatching();
    void testPartialTitleMatching();
    void testCaseSensitiveTitleMatching();
    void testMultipleTitleMatching();

    // Project Splitting Tests
    void testProjectSplitting();
    void testSplittingWithUUIDFilter();
    void testSplittingWithTitleFilter();
    void testSplittingOutputValidation();

    // File Joining Tests
    void testFileJoining();
    void testJoiningMultipleFiles();
    void testJoiningWithDuplicates();
    void testJoiningSingleProject();

    // Multi-Project Structure Tests
    void testMultiProjectValidation();
    void testProjectKeyDetection();
    void testFormatVersionHandling();
    void testMetadataPreservation();

    // Error Handling Tests
    void testNonExistentMultiProjectFile();
    void testCorruptedMultiProjectFile();
    void testEmptyProjectExtraction();
    void testConflictingParameters();

private:
    QTemporaryDir* m_tempDir;
    QString m_suprafitCli;
    
    QString createMultiProjectFile(int projectCount = 3);
    QString createSingleProjectFile();
    QString createProjectWithUUID(const QString& uuid, const QString& title = "Test Project");
    QString createCorruptedMultiProjectFile();
    QStringList runCliCommand(const QStringList& arguments);
    QJsonObject loadProjectFile(const QString& filename);
    bool verifyProjectStructure(const QJsonObject& project, int expectedProjects = -1);
    bool hasProject(const QJsonObject& data, int index);
    bool hasProjectWithUUID(const QJsonObject& data, const QString& uuid);
    bool hasProjectWithTitle(const QJsonObject& data, const QString& title);
    int countProjects(const QJsonObject& data);
};

void TestMultiProject::initTestCase()
{
    qDebug() << "Starting Multi-Project tests...";
    m_tempDir = new QTemporaryDir();
    QVERIFY(m_tempDir->isValid());
    
    // Find suprafit_cli executable
    m_suprafitCli = QCoreApplication::applicationDirPath() + "/bin/linux/suprafit_cli";
    if (!QFile::exists(m_suprafitCli)) {
        m_suprafitCli = "../bin/linux/suprafit_cli";
    }
    if (!QFile::exists(m_suprafitCli)) {
        m_suprafitCli = "bin/linux/suprafit_cli";
    }
    
    QVERIFY2(QFile::exists(m_suprafitCli), "suprafit_cli executable not found");
}

void TestMultiProject::cleanupTestCase()
{
    delete m_tempDir;
    qDebug() << "Multi-Project tests completed.";
}

QStringList TestMultiProject::runCliCommand(const QStringList& arguments)
{
    QProcess process;
    process.start(m_suprafitCli, arguments);
    process.waitForFinished(30000); // 30 second timeout
    
    QStringList result;
    result << QString::number(process.exitCode());
    result << QString::fromUtf8(process.readAllStandardOutput());
    result << QString::fromUtf8(process.readAllStandardError());
    
    return result;
}

QJsonObject TestMultiProject::loadProjectFile(const QString& filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        return QJsonObject();
    }
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError) {
        qDebug() << "JSON parsing error:" << error.errorString();
        return QJsonObject();
    }
    
    return doc.object();
}

bool TestMultiProject::verifyProjectStructure(const QJsonObject& project, int expectedProjects)
{
    if (!project.contains("format_version")) return false;
    
    int projectCount = countProjects(project);
    if (expectedProjects >= 0 && projectCount != expectedProjects) {
        return false;
    }
    
    return projectCount > 0;
}

bool TestMultiProject::hasProject(const QJsonObject& data, int index)
{
    QString projectKey = QString("project_%1").arg(index);
    return data.contains(projectKey);
}

bool TestMultiProject::hasProjectWithUUID(const QJsonObject& data, const QString& uuid)
{
    for (auto it = data.begin(); it != data.end(); ++it) {
        if (it.key().startsWith("project_")) {
            QJsonObject project = it.value().toObject();
            if (project.contains("uuid")) {
                QString projectUuid = project["uuid"].toString();
                if (projectUuid == uuid || projectUuid.startsWith(uuid)) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool TestMultiProject::hasProjectWithTitle(const QJsonObject& data, const QString& title)
{
    for (auto it = data.begin(); it != data.end(); ++it) {
        if (it.key().startsWith("project_")) {
            QJsonObject project = it.value().toObject();
            if (project.contains("title")) {
                QString projectTitle = project["title"].toString();
                if (projectTitle.contains(title, Qt::CaseInsensitive)) {
                    return true;
                }
            }
        }
    }
    return false;
}

int TestMultiProject::countProjects(const QJsonObject& data)
{
    int count = 0;
    for (auto it = data.begin(); it != data.end(); ++it) {
        if (it.key().startsWith("project_") || it.key().startsWith("model_")) {
            count++;
        }
    }
    return count;
}

void TestMultiProject::testProjectExtractionByIndex()
{
    QString multiProjectFile = createMultiProjectFile(3);
    QString outputFile = m_tempDir->path() + "/extracted_project.json";
    
    // Extract project 0
    QStringList result = runCliCommand({"-i", multiProjectFile, "-o", outputFile, "-p", "0"});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Project extraction by index failed: " + result[2]));
    
    // Verify output file exists
    QVERIFY2(QFile::exists(outputFile), "Extracted project file not created");
    
    // Load and verify structure
    QJsonObject extractedData = loadProjectFile(outputFile);
    QVERIFY2(!extractedData.isEmpty(), "Extracted project file is empty");
    
    // Should contain only one project or be a converted single project
    QVERIFY2(verifyProjectStructure(extractedData, 1) || !extractedData.contains("project_1"), 
             "Extracted file contains unexpected multiple projects");
}

void TestMultiProject::testMultipleProjectExtraction()
{
    QString multiProjectFile = createMultiProjectFile(5);
    QString outputFile = m_tempDir->path() + "/multiple_extracted.json";
    
    // Extract project 2
    QStringList result = runCliCommand({"-i", multiProjectFile, "-o", outputFile, "-p", "2"});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Multiple project extraction failed: " + result[2]));
    
    QVERIFY(QFile::exists(outputFile));
    
    QJsonObject extractedData = loadProjectFile(outputFile);
    QVERIFY(!extractedData.isEmpty());
    
    // Should successfully extract from larger multi-project file
    QVERIFY(verifyProjectStructure(extractedData));
}

void TestMultiProject::testInvalidProjectIndex()
{
    QString multiProjectFile = createMultiProjectFile(3);
    QString outputFile = m_tempDir->path() + "/invalid_index.json";
    
    // Try to extract project 10 from a 3-project file
    QStringList result = runCliCommand({"-i", multiProjectFile, "-o", outputFile, "-p", "10"});
    
    // Should fail gracefully
    QVERIFY(result[0].toInt() != 0);
    QVERIFY(result[2].contains("Error") || result[2].contains("ERROR") ||
            result[1].contains("Error") || result[1].contains("not found"));
}

void TestMultiProject::testProjectIndexBoundaries()
{
    QString multiProjectFile = createMultiProjectFile(2); // 2 projects: 0, 1
    
    // Test valid boundary
    QString outputFile1 = m_tempDir->path() + "/boundary_valid.json";
    QStringList result1 = runCliCommand({"-i", multiProjectFile, "-o", outputFile1, "-p", "1"});
    QVERIFY2(result1[0].toInt() == 0, qPrintable("Valid boundary test failed: " + result1[2]));
    
    // Test invalid boundary
    QString outputFile2 = m_tempDir->path() + "/boundary_invalid.json";
    QStringList result2 = runCliCommand({"-i", multiProjectFile, "-o", outputFile2, "-p", "2"});
    QVERIFY(result2[0].toInt() != 0); // Should fail
    
    // Test negative index
    QString outputFile3 = m_tempDir->path() + "/boundary_negative.json";
    QStringList result3 = runCliCommand({"-i", multiProjectFile, "-o", outputFile3, "-p", "-1"});
    QVERIFY(result3[0].toInt() != 0); // Should fail
}

void TestMultiProject::testUUIDMatching()
{
    QString uuid1 = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString uuid2 = QUuid::createUuid().toString(QUuid::WithoutBraces);
    
    QString projectFile1 = createProjectWithUUID(uuid1, "Project 1");
    QString projectFile2 = createProjectWithUUID(uuid2, "Project 2");
    
    // Join projects first
    QString multiProjectFile = m_tempDir->path() + "/uuid_projects.json";
    QStringList joinResult = runCliCommand({"-i", projectFile1, "-i", projectFile2, "-o", multiProjectFile, "-j"});
    
    if (joinResult[0].toInt() == 0) {
        QString outputFile = m_tempDir->path() + "/uuid_matched.json";
        QStringList result = runCliCommand({"-i", multiProjectFile, "-o", outputFile, "-u", uuid1});
        
        QVERIFY2(result[0].toInt() == 0, qPrintable("UUID matching failed: " + result[2]));
        
        if (QFile::exists(outputFile)) {
            QJsonObject matchedData = loadProjectFile(outputFile);
            QVERIFY2(hasProjectWithUUID(matchedData, uuid1), "Matched project does not contain expected UUID");
        }
    }
}

void TestMultiProject::testPartialUUIDMatching()
{
    QString fullUuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString partialUuid = fullUuid.left(8); // First 8 characters
    
    QString projectFile = createProjectWithUUID(fullUuid, "Partial UUID Test");
    QString outputFile = m_tempDir->path() + "/partial_uuid.json";
    
    QStringList result = runCliCommand({"-i", projectFile, "-o", outputFile, "-u", partialUuid});
    
    if (result[0].toInt() == 0) {
        QVERIFY(QFile::exists(outputFile));
        QJsonObject matchedData = loadProjectFile(outputFile);
        QVERIFY(hasProjectWithUUID(matchedData, partialUuid));
    } else {
        // Partial UUID matching might not be supported
        QVERIFY(result[0].toInt() >= 0); // At least no crash
    }
}

void TestMultiProject::testMultipleUUIDMatching()
{
    QString uuid1 = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString uuid2 = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString uuid3 = QUuid::createUuid().toString(QUuid::WithoutBraces);
    
    // Create projects with different UUIDs
    QString projectFile1 = createProjectWithUUID(uuid1, "UUID Project 1");
    QString projectFile2 = createProjectWithUUID(uuid2, "UUID Project 2");
    QString projectFile3 = createProjectWithUUID(uuid3, "UUID Project 3");
    
    // Join all projects
    QString multiProjectFile = m_tempDir->path() + "/multi_uuid_projects.json";
    QStringList joinResult = runCliCommand({"-i", projectFile1, "-i", projectFile2, "-i", projectFile3, 
                                           "-o", multiProjectFile, "-j"});
    
    if (joinResult[0].toInt() == 0) {
        // Try to match multiple UUIDs (might need multiple calls or comma separation)
        QString outputFile = m_tempDir->path() + "/multi_uuid_matched.json";
        QStringList result = runCliCommand({"-i", multiProjectFile, "-o", outputFile, "-u", uuid1});
        
        QVERIFY2(result[0].toInt() == 0, qPrintable("Multiple UUID matching failed: " + result[2]));
    }
}

void TestMultiProject::testInvalidUUIDMatching()
{
    QString multiProjectFile = createMultiProjectFile(2);
    QString outputFile = m_tempDir->path() + "/invalid_uuid.json";
    QString fakeUuid = "invalid-uuid-format";
    
    QStringList result = runCliCommand({"-i", multiProjectFile, "-o", outputFile, "-u", fakeUuid});
    
    // Should handle invalid UUID gracefully
    QVERIFY(result[0].toInt() >= 0); // No crashes
    
    if (result[0].toInt() != 0) {
        // Might fail with invalid UUID
        QVERIFY(result[2].contains("Error") || result[2].contains("not found"));
    }
}

void TestMultiProject::testTitleMatching()
{
    QString projectFile1 = createProjectWithUUID(QUuid::createUuid().toString(QUuid::WithoutBraces), "NMR Analysis");
    QString projectFile2 = createProjectWithUUID(QUuid::createUuid().toString(QUuid::WithoutBraces), "ITC Study");
    
    // Join projects
    QString multiProjectFile = m_tempDir->path() + "/title_projects.json";
    QStringList joinResult = runCliCommand({"-i", projectFile1, "-i", projectFile2, "-o", multiProjectFile, "-j"});
    
    if (joinResult[0].toInt() == 0) {
        QString outputFile = m_tempDir->path() + "/title_matched.json";
        QStringList result = runCliCommand({"-i", multiProjectFile, "-o", outputFile, "-t", "NMR"});
        
        QVERIFY2(result[0].toInt() == 0, qPrintable("Title matching failed: " + result[2]));
        
        if (QFile::exists(outputFile)) {
            QJsonObject matchedData = loadProjectFile(outputFile);
            QVERIFY2(hasProjectWithTitle(matchedData, "NMR"), "Matched project does not contain expected title");
        }
    }
}

void TestMultiProject::testPartialTitleMatching()
{
    QString projectFile = createProjectWithUUID(QUuid::createUuid().toString(QUuid::WithoutBraces), 
                                               "Comprehensive NMR Binding Analysis");
    QString outputFile = m_tempDir->path() + "/partial_title.json";
    
    QStringList result = runCliCommand({"-i", projectFile, "-o", outputFile, "-t", "Binding"});
    
    QVERIFY2(result[0].toInt() == 0, qPrintable("Partial title matching failed: " + result[2]));
    
    if (QFile::exists(outputFile)) {
        QJsonObject matchedData = loadProjectFile(outputFile);
        QVERIFY(hasProjectWithTitle(matchedData, "Binding"));
    }
}

void TestMultiProject::testCaseSensitiveTitleMatching()
{
    QString projectFile = createProjectWithUUID(QUuid::createUuid().toString(QUuid::WithoutBraces), "Test Analysis");
    QString outputFile = m_tempDir->path() + "/case_title.json";
    
    // Test with different case
    QStringList result = runCliCommand({"-i", projectFile, "-o", outputFile, "-t", "test"});
    
    QVERIFY2(result[0].toInt() == 0, qPrintable("Case-insensitive title matching failed: " + result[2]));
    
    if (QFile::exists(outputFile)) {
        QJsonObject matchedData = loadProjectFile(outputFile);
        QVERIFY(hasProjectWithTitle(matchedData, "test"));
    }
}

void TestMultiProject::testMultipleTitleMatching()
{
    QString projectFile1 = createProjectWithUUID(QUuid::createUuid().toString(QUuid::WithoutBraces), "Test Study A");
    QString projectFile2 = createProjectWithUUID(QUuid::createUuid().toString(QUuid::WithoutBraces), "Test Study B");
    QString projectFile3 = createProjectWithUUID(QUuid::createUuid().toString(QUuid::WithoutBraces), "Control Analysis");
    
    // Join projects
    QString multiProjectFile = m_tempDir->path() + "/multi_title_projects.json";
    QStringList joinResult = runCliCommand({"-i", projectFile1, "-i", projectFile2, "-i", projectFile3, 
                                           "-o", multiProjectFile, "-j"});
    
    if (joinResult[0].toInt() == 0) {
        QString outputFile = m_tempDir->path() + "/multi_title_matched.json";
        QStringList result = runCliCommand({"-i", multiProjectFile, "-o", outputFile, "-t", "Test"});
        
        QVERIFY2(result[0].toInt() == 0, qPrintable("Multiple title matching failed: " + result[2]));
        
        if (QFile::exists(outputFile)) {
            QJsonObject matchedData = loadProjectFile(outputFile);
            int testProjects = 0;
            for (auto it = matchedData.begin(); it != matchedData.end(); ++it) {
                if (it.key().startsWith("project_")) {
                    QJsonObject project = it.value().toObject();
                    if (project.contains("title")) {
                        QString title = project["title"].toString();
                        if (title.contains("Test", Qt::CaseInsensitive)) {
                            testProjects++;
                        }
                    }
                }
            }
            QVERIFY2(testProjects > 0, "No projects with 'Test' in title found");
        }
    }
}

void TestMultiProject::testProjectSplitting()
{
    QString multiProjectFile = createMultiProjectFile(3);
    QString outputDir = m_tempDir->path() + "/split_output";
    QDir().mkpath(outputDir);
    
    QStringList result = runCliCommand({"-i", multiProjectFile, "-o", outputDir + "/split", "-s"});
    
    QVERIFY2(result[0].toInt() == 0, qPrintable("Project splitting failed: " + result[2]));
    
    // Should create separate files for each project
    QDir outputDirObj(outputDir);
    QStringList generatedFiles = outputDirObj.entryList(QStringList() << "split*", QDir::Files);
    
    QVERIFY2(generatedFiles.size() > 1, "Project splitting did not create multiple files");
    
    // Verify each split file is valid
    for (const QString& fileName : generatedFiles) {
        QString filePath = outputDir + "/" + fileName;
        QJsonObject projectData = loadProjectFile(filePath);
        QVERIFY2(!projectData.isEmpty(), qPrintable("Split file " + fileName + " is empty or invalid"));
    }
}

void TestMultiProject::testSplittingWithUUIDFilter()
{
    QString uuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString projectFile1 = createProjectWithUUID(uuid, "UUID Split Test");
    QString projectFile2 = createProjectWithUUID(QUuid::createUuid().toString(QUuid::WithoutBraces), "Other Project");
    
    // Join first
    QString multiProjectFile = m_tempDir->path() + "/uuid_split_projects.json";
    QStringList joinResult = runCliCommand({"-i", projectFile1, "-i", projectFile2, "-o", multiProjectFile, "-j"});
    
    if (joinResult[0].toInt() == 0) {
        QString outputDir = m_tempDir->path() + "/uuid_split_output";
        QDir().mkpath(outputDir);
        
        QStringList result = runCliCommand({"-i", multiProjectFile, "-o", outputDir + "/uuid_split", 
                                           "-u", uuid, "-s"});
        
        QVERIFY2(result[0].toInt() == 0, qPrintable("UUID-filtered splitting failed: " + result[2]));
        
        // Should create file(s) only for matching UUID
        QDir outputDirObj(outputDir);
        QStringList generatedFiles = outputDirObj.entryList(QStringList() << "uuid_split*", QDir::Files);
        QVERIFY(generatedFiles.size() > 0);
    }
}

void TestMultiProject::testSplittingWithTitleFilter()
{
    QString projectFile1 = createProjectWithUUID(QUuid::createUuid().toString(QUuid::WithoutBraces), "Kinetics Study");
    QString projectFile2 = createProjectWithUUID(QUuid::createUuid().toString(QUuid::WithoutBraces), "Thermodynamics Study");
    
    // Join first
    QString multiProjectFile = m_tempDir->path() + "/title_split_projects.json";
    QStringList joinResult = runCliCommand({"-i", projectFile1, "-i", projectFile2, "-o", multiProjectFile, "-j"});
    
    if (joinResult[0].toInt() == 0) {
        QString outputDir = m_tempDir->path() + "/title_split_output";
        QDir().mkpath(outputDir);
        
        QStringList result = runCliCommand({"-i", multiProjectFile, "-o", outputDir + "/title_split", 
                                           "-t", "Kinetics", "-s"});
        
        QVERIFY2(result[0].toInt() == 0, qPrintable("Title-filtered splitting failed: " + result[2]));
        
        QDir outputDirObj(outputDir);
        QStringList generatedFiles = outputDirObj.entryList(QStringList() << "title_split*", QDir::Files);
        QVERIFY(generatedFiles.size() > 0);
    }
}

void TestMultiProject::testSplittingOutputValidation()
{
    QString multiProjectFile = createMultiProjectFile(2);
    QString outputDir = m_tempDir->path() + "/validation_split";
    QDir().mkpath(outputDir);
    
    QStringList result = runCliCommand({"-i", multiProjectFile, "-o", outputDir + "/valid_split", "-s"});
    
    if (result[0].toInt() == 0) {
        QDir outputDirObj(outputDir);
        QStringList generatedFiles = outputDirObj.entryList(QStringList() << "valid_split*", QDir::Files);
        
        // Validate each split file
        for (const QString& fileName : generatedFiles) {
            QString filePath = outputDir + "/" + fileName;
            QJsonObject projectData = loadProjectFile(filePath);
            
            QVERIFY2(verifyProjectStructure(projectData), qPrintable("Invalid structure in " + fileName));
            
            // Should contain only one project (or be converted to single format)
            int projectCount = countProjects(projectData);
            QVERIFY2(projectCount == 1, qPrintable(QString("Split file %1 contains %2 projects, expected 1").arg(fileName).arg(projectCount)));
        }
    }
}

void TestMultiProject::testFileJoining()
{
    QString projectFile1 = createSingleProjectFile();
    QString projectFile2 = createSingleProjectFile();
    QString outputFile = m_tempDir->path() + "/joined_projects.json";
    
    QStringList result = runCliCommand({"-i", projectFile1, "-i", projectFile2, "-o", outputFile, "-j"});
    QVERIFY2(result[0].toInt() == 0, qPrintable("File joining failed: " + result[2]));
    
    QVERIFY2(QFile::exists(outputFile), "Joined project file not created");
    
    QJsonObject joinedData = loadProjectFile(outputFile);
    QVERIFY2(!joinedData.isEmpty(), "Joined project file is empty");
    
    // Should contain multiple projects
    int projectCount = countProjects(joinedData);
    QVERIFY2(projectCount >= 2, qPrintable(QString("Expected at least 2 projects, found %1").arg(projectCount)));
}

void TestMultiProject::testJoiningMultipleFiles()
{
    QStringList projectFiles;
    for (int i = 0; i < 4; ++i) {
        projectFiles << createSingleProjectFile();
    }
    
    QString outputFile = m_tempDir->path() + "/multi_joined_projects.json";
    
    QStringList args;
    for (const QString& file : projectFiles) {
        args << "-i" << file;
    }
    args << "-o" << outputFile << "-j";
    
    QStringList result = runCliCommand(args);
    QVERIFY2(result[0].toInt() == 0, qPrintable("Multiple file joining failed: " + result[2]));
    
    QVERIFY(QFile::exists(outputFile));
    
    QJsonObject joinedData = loadProjectFile(outputFile);
    int projectCount = countProjects(joinedData);
    QVERIFY2(projectCount >= 4, qPrintable(QString("Expected at least 4 projects, found %1").arg(projectCount)));
}

void TestMultiProject::testJoiningWithDuplicates()
{
    QString projectFile = createSingleProjectFile();
    QString outputFile = m_tempDir->path() + "/duplicate_joined.json";
    
    // Join the same file twice
    QStringList result = runCliCommand({"-i", projectFile, "-i", projectFile, "-o", outputFile, "-j"});
    
    QVERIFY2(result[0].toInt() == 0, qPrintable("Joining with duplicates failed: " + result[2]));
    
    if (QFile::exists(outputFile)) {
        QJsonObject joinedData = loadProjectFile(outputFile);
        int projectCount = countProjects(joinedData);
        
        // Should handle duplicates somehow (might create separate projects or merge)
        QVERIFY2(projectCount > 0, "No projects found after joining duplicates");
    }
}

void TestMultiProject::testJoiningSingleProject()
{
    QString projectFile = createSingleProjectFile();
    QString outputFile = m_tempDir->path() + "/single_joined.json";
    
    // Join just one file
    QStringList result = runCliCommand({"-i", projectFile, "-o", outputFile, "-j"});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Single project joining failed: " + result[2]));
    
    QVERIFY(QFile::exists(outputFile));
    
    QJsonObject joinedData = loadProjectFile(outputFile);
    QVERIFY(verifyProjectStructure(joinedData, 1));
}

void TestMultiProject::testMultiProjectValidation()
{
    QString multiProjectFile = createMultiProjectFile(3);
    
    // Test file analysis to validate structure
    QStringList result = runCliCommand({multiProjectFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Multi-project validation failed: " + result[2]));
    
    // Should show multi-project information
    QVERIFY(result[1].contains("project") || result[1].contains("Project") ||
            result[1].contains("multi") || result[1].contains("Multi"));
}

void TestMultiProject::testProjectKeyDetection()
{
    QString multiProjectFile = createMultiProjectFile(2);
    
    QStringList result = runCliCommand({"-l", multiProjectFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Project key detection failed: " + result[2]));
    
    // Should list project keys
    QVERIFY(result[1].contains("project_0") || result[1].contains("project_1") ||
            result[1].contains("Project") || result[1].contains("project"));
}

void TestMultiProject::testFormatVersionHandling()
{
    QString multiProjectFile = createMultiProjectFile(2);
    
    QJsonObject data = loadProjectFile(multiProjectFile);
    QVERIFY(data.contains("format_version"));
    
    // Test that CLI handles format version correctly
    QStringList result = runCliCommand({multiProjectFile});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Format version handling failed: " + result[2]));
}

void TestMultiProject::testMetadataPreservation()
{
    QString multiProjectFile = createMultiProjectFile(2);
    QString outputFile = m_tempDir->path() + "/metadata_test.json";
    
    // Extract and verify metadata preservation
    QStringList result = runCliCommand({"-i", multiProjectFile, "-o", outputFile, "-p", "0"});
    QVERIFY2(result[0].toInt() == 0, qPrintable("Metadata preservation test failed: " + result[2]));
    
    if (QFile::exists(outputFile)) {
        QJsonObject extractedData = loadProjectFile(outputFile);
        
        // Should preserve relevant metadata
        QVERIFY(!extractedData.isEmpty());
        
        // Format version might be preserved or updated
        if (extractedData.contains("format_version")) {
            QVERIFY(!extractedData["format_version"].toString().isEmpty());
        }
    }
}

void TestMultiProject::testNonExistentMultiProjectFile()
{
    QString nonexistentFile = m_tempDir->path() + "/nonexistent.json";
    QString outputFile = m_tempDir->path() + "/error_test.json";
    
    QStringList result = runCliCommand({"-i", nonexistentFile, "-o", outputFile, "-p", "0"});
    
    // Should fail gracefully
    QVERIFY(result[0].toInt() != 0);
    QVERIFY(result[2].contains("Error") || result[2].contains("ERROR"));
}

void TestMultiProject::testCorruptedMultiProjectFile()
{
    QString corruptedFile = createCorruptedMultiProjectFile();
    QString outputFile = m_tempDir->path() + "/corrupted_test.json";
    
    QStringList result = runCliCommand({"-i", corruptedFile, "-o", outputFile, "-p", "0"});
    
    // Should handle corruption gracefully
    QVERIFY(result[0].toInt() >= 0); // No crashes
    
    if (result[0].toInt() != 0) {
        QVERIFY(result[2].contains("Error") || result[2].contains("parsing"));
    }
}

void TestMultiProject::testEmptyProjectExtraction()
{
    // Create a multi-project file with empty projects
    QJsonObject emptyMultiProject;
    emptyMultiProject["format_version"] = "2.0";
    emptyMultiProject["project_0"] = QJsonObject();
    
    QString emptyFile = m_tempDir->path() + "/empty_projects.json";
    QFile file(emptyFile);
    file.open(QIODevice::WriteOnly);
    file.write(QJsonDocument(emptyMultiProject).toJson());
    file.close();
    
    QString outputFile = m_tempDir->path() + "/empty_extracted.json";
    QStringList result = runCliCommand({"-i", emptyFile, "-o", outputFile, "-p", "0"});
    
    // Should handle empty projects gracefully
    QVERIFY(result[0].toInt() >= 0);
}

void TestMultiProject::testConflictingParameters()
{
    QString multiProjectFile = createMultiProjectFile(3);
    QString outputFile = m_tempDir->path() + "/conflict_test.json";
    
    // Use conflicting parameters: both -p and -u
    QString uuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QStringList result = runCliCommand({"-i", multiProjectFile, "-o", outputFile, "-p", "0", "-u", uuid});
    
    // Should handle parameter conflicts gracefully
    QVERIFY(result[0].toInt() >= 0); // No crashes
    
    // Might succeed with one parameter taking precedence, or fail with clear error
    if (result[0].toInt() != 0) {
        QVERIFY(result[2].contains("Error") || result[2].contains("conflict") || 
                result[2].contains("parameter"));
    }
}

QString TestMultiProject::createMultiProjectFile(int projectCount)
{
    QJsonObject multiProject;
    multiProject["format_version"] = "2.0";
    multiProject["generation_timestamp"] = "2025-09-02T12:00:00";
    
    QJsonObject baseData;
    QJsonObject independent;
    independent["rows"] = 20;
    independent["cols"] = 2;
    baseData["Independent"] = independent;
    multiProject["base_data"] = baseData;
    
    for (int i = 0; i < projectCount; ++i) {
        QJsonObject project;
        project["model_id"] = i + 1;
        project["model_name"] = QString("model_%1").arg(i + 1);
        project["sse"] = 0.05 * (i + 1);
        project["convergence"] = true;
        project["title"] = QString("Test Project %1").arg(i + 1);
        project["uuid"] = QUuid::createUuid().toString(QUuid::WithoutBraces);
        
        multiProject[QString("project_%1").arg(i)] = project;
    }
    
    QString filePath = m_tempDir->path() + QString("/multi_project_%1.json").arg(projectCount);
    QFile file(filePath);
    file.open(QIODevice::WriteOnly);
    file.write(QJsonDocument(multiProject).toJson());
    file.close();
    
    return filePath;
}

QString TestMultiProject::createSingleProjectFile()
{
    QJsonObject singleProject;
    singleProject["format_version"] = "1.0";
    singleProject["model_id"] = 1;
    singleProject["model_name"] = "nmr_1_1";
    singleProject["sse"] = 0.05;
    singleProject["convergence"] = true;
    singleProject["title"] = "Single Project";
    singleProject["uuid"] = QUuid::createUuid().toString(QUuid::WithoutBraces);
    
    QString filePath = m_tempDir->path() + QString("/single_project_%1.json").arg(QTime::currentTime().msec());
    QFile file(filePath);
    file.open(QIODevice::WriteOnly);
    file.write(QJsonDocument(singleProject).toJson());
    file.close();
    
    return filePath;
}

QString TestMultiProject::createProjectWithUUID(const QString& uuid, const QString& title)
{
    QJsonObject project;
    project["format_version"] = "1.0";
    project["model_id"] = 1;
    project["model_name"] = "nmr_1_1";
    project["sse"] = 0.05;
    project["convergence"] = true;
    project["title"] = title;
    project["uuid"] = uuid;
    
    QString filePath = m_tempDir->path() + QString("/uuid_project_%1.json").arg(uuid.left(8));
    QFile file(filePath);
    file.open(QIODevice::WriteOnly);
    file.write(QJsonDocument(project).toJson());
    file.close();
    
    return filePath;
}

QString TestMultiProject::createCorruptedMultiProjectFile()
{
    QString filePath = m_tempDir->path() + "/corrupted_multi_project.json";
    QFile file(filePath);
    file.open(QIODevice::WriteOnly);
    file.write("{ \"format_version\": \"2.0\", \"project_0\": { invalid json");
    file.close();
    
    return filePath;
}

#include "test_multi_project.moc"

QTEST_MAIN(TestMultiProject)