/*
 * XOR Tutorial Unit Tests
 * Copyright (C) 2019 - 2025 Conrad Hübler <Conrad.Huebler@gmx.net>
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
 * 
 */

#include <QtTest/QtTest>
#include <QObject>
#include <QJsonObject>

#include "../ml/tutorial/examples/xor_tutorial.h"

/**
 * @brief Comprehensive test suite for XOR Tutorial
 * 
 * Tests the educational XOR neural network implementation to ensure
 * it correctly demonstrates non-linear problem solving with hidden layers.
 * 
 * Claude Generated - 2025
 */
class TestXORTutorial : public QObject
{
    Q_OBJECT

private slots:
    void testTutorialConfiguration();
    void testTrainingDataGeneration();
    void testSimplePerceptronFailure();
    void testPretrainedXORNetwork();
    void testXORSolutionValidation();
    void testTutorialExport();
    void testNetworkArchitecture();
    void testEducationalContent();

private:
    bool isApproximatelyEqual(double a, double b, double tolerance = 0.15);  // Generous tolerance for XOR
    XORTutorial::TutorialConfig createNonInteractiveConfig();
};

void TestXORTutorial::testTutorialConfiguration()
{
    // Test default configuration
    XORTutorial tutorial_default;
    
    // Test custom configuration
    XORTutorial::TutorialConfig config;
    config.interactive = false;
    config.show_matrices = false;
    config.show_calculations = false;
    config.step_by_step = false;
    config.detail_level = TutorialLevel::Basic;
    
    XORTutorial tutorial_custom(config);
    
    // Both should construct without errors
    QVERIFY(tutorial_custom.validateXORSolution());
    QVERIFY(tutorial_default.validateXORSolution());
}

void TestXORTutorial::testTrainingDataGeneration()
{
    XORTutorial tutorial(createNonInteractiveConfig());
    
    auto training_data = tutorial.createXORTrainingData();
    
    // Should have exactly 4 samples (XOR truth table)
    QCOMPARE(training_data.size(), static_cast<size_t>(4));
    
    // Verify XOR truth table
    bool found_00 = false, found_01 = false, found_10 = false, found_11 = false;
    
    for (const auto& sample : training_data) {
        QCOMPARE(sample.input.size(), 2);
        QVERIFY(!sample.description.isEmpty());
        
        // Check input combinations and expected outputs
        if (sample.input(0) == 0.0 && sample.input(1) == 0.0) {
            QCOMPARE(sample.expected_output, 0.0);  // 0 XOR 0 = 0
            found_00 = true;
        } else if (sample.input(0) == 0.0 && sample.input(1) == 1.0) {
            QCOMPARE(sample.expected_output, 1.0);  // 0 XOR 1 = 1
            found_01 = true;
        } else if (sample.input(0) == 1.0 && sample.input(1) == 0.0) {
            QCOMPARE(sample.expected_output, 1.0);  // 1 XOR 0 = 1
            found_10 = true;
        } else if (sample.input(0) == 1.0 && sample.input(1) == 1.0) {
            QCOMPARE(sample.expected_output, 0.0);  // 1 XOR 1 = 0
            found_11 = true;
        }
    }
    
    // Verify all combinations were found
    QVERIFY(found_00 && found_01 && found_10 && found_11);
}

void TestXORTutorial::testSimplePerceptronFailure()
{
    XORTutorial tutorial(createNonInteractiveConfig());
    
    // Create simple perceptron (should fail on XOR)
    NeuralNetwork perceptron = tutorial.createSimplePerceptron();
    
    // Verify it's a single-layer network
    QCOMPARE(perceptron.layerCount(), static_cast<size_t>(1));
    
    auto architecture = perceptron.getArchitecture();
    QCOMPARE(architecture.size(), static_cast<size_t>(2));  // Input + output
    QCOMPARE(architecture[0], 2);  // 2 inputs
    QCOMPARE(architecture[1], 1);  // 1 output
    
    // Test that perceptron fails on XOR (at least one case should be wrong)
    auto training_data = tutorial.createXORTrainingData();
    int correct_predictions = 0;
    
    for (const auto& sample : training_data) {
        auto prediction = perceptron.predict(sample.input);
        double output = prediction.size() > 0 ? prediction(0) : 0.0;
        
        // Convert to binary prediction (threshold at 0.5)
        double predicted_value = output >= 0.5 ? 1.0 : 0.0;
        
        if (std::abs(predicted_value - sample.expected_output) < 0.1) {
            correct_predictions++;
        }
    }
    
    // Perceptron should not solve XOR perfectly (should get at most 2/4 correct)
    QVERIFY(correct_predictions < 4);
}

void TestXORTutorial::testPretrainedXORNetwork()
{
    XORTutorial tutorial(createNonInteractiveConfig());
    
    NeuralNetwork network = tutorial.createPretrainedXORNetwork();
    
    // Verify network architecture
    QCOMPARE(network.layerCount(), static_cast<size_t>(2));
    
    auto architecture = network.getArchitecture();
    QCOMPARE(architecture.size(), static_cast<size_t>(3));
    QCOMPARE(architecture[0], 2);  // 2 inputs (A, B)
    QCOMPARE(architecture[1], 2);  // 2 hidden neurons
    QCOMPARE(architecture[2], 1);  // 1 output
    
    // Test that network correctly solves XOR
    auto training_data = tutorial.createXORTrainingData();
    
    for (const auto& sample : training_data) {
        auto prediction = network.predict(sample.input);
        double output = prediction.size() > 0 ? prediction(0) : 0.0;
        
        // Convert to binary prediction
        double predicted_value = output >= 0.5 ? 1.0 : 0.0;
        
        QVERIFY(isApproximatelyEqual(predicted_value, sample.expected_output));
    }
}

void TestXORTutorial::testXORSolutionValidation()
{
    XORTutorial tutorial(createNonInteractiveConfig());
    
    // Validate the XOR solution
    QVERIFY(tutorial.validateXORSolution());
    
    // Test individual validation with different tolerances
    NeuralNetwork network = tutorial.createPretrainedXORNetwork();
    
    // Should pass with reasonable tolerance
    QVERIFY(tutorial.testNetwork(network, 0.2));
    
    // Should pass with strict tolerance
    QVERIFY(tutorial.testNetwork(network, 0.1));
    
    // Should pass even with very strict tolerance (good pre-trained network)
    QVERIFY(tutorial.testNetwork(network, 0.05));
}

void TestXORTutorial::testTutorialExport()
{
    XORTutorial tutorial(createNonInteractiveConfig());
    
    QJsonObject export_data = tutorial.exportTutorialResults();
    
    // Verify export structure
    QVERIFY(export_data.contains("tutorial_name"));
    QVERIFY(export_data.contains("network_architecture"));
    QVERIFY(export_data.contains("activation_functions"));
    QVERIFY(export_data.contains("validation_passed"));
    QVERIFY(export_data.contains("educational_goal"));
    
    // Verify content
    QCOMPARE(export_data["tutorial_name"].toString(), "XOR Problem");
    QCOMPARE(export_data["network_architecture"].toString(), "2-2-1");
    QVERIFY(export_data["activation_functions"].toString().contains("ReLU"));
    QVERIFY(export_data["activation_functions"].toString().contains("Sigmoid"));
    QVERIFY(export_data["validation_passed"].toBool());
    QVERIFY(!export_data["educational_goal"].toString().isEmpty());
}

void TestXORTutorial::testNetworkArchitecture()
{
    XORTutorial tutorial(createNonInteractiveConfig());
    
    NeuralNetwork network = tutorial.createPretrainedXORNetwork();
    
    // Test network information
    QString network_info = network.getNetworkInfo();
    QVERIFY(network_info.contains("2"));  // Input size
    QVERIFY(network_info.contains("1"));  // Output size
    
    // Test parameter count
    size_t total_params = network.getTotalParameters();
    // Expected: (2*2 + 2) + (2*1 + 1) = 4 + 2 + 2 + 1 = 9
    QCOMPARE(total_params, static_cast<size_t>(9));
    
    // Test memory usage
    double memory_mb = network.getMemoryUsageMB();
    QVERIFY(memory_mb > 0.0);
}

void TestXORTutorial::testEducationalContent()
{
    // Test that tutorial methods don't crash when called
    XORTutorial tutorial(createNonInteractiveConfig());
    
    // These should execute without throwing exceptions
    try {
        tutorial.explainXORProblem();
        tutorial.demonstratePerceptronFailure();
        tutorial.buildNeuralNetworkSolution();
        tutorial.testNetworkOnAllInputs();
        tutorial.explainWhyItWorks();
        
        // Test full tutorial (non-interactive)
        tutorial.runFullTutorial();
        
        QVERIFY(true);  // If we get here, no exceptions were thrown
    } catch (...) {
        QFAIL("Tutorial methods should not throw exceptions");
    }
    
    // Test that validation still works after running tutorial
    QVERIFY(tutorial.validateXORSolution());
}

bool TestXORTutorial::isApproximatelyEqual(double a, double b, double tolerance)
{
    return std::abs(a - b) < tolerance;
}

XORTutorial::TutorialConfig TestXORTutorial::createNonInteractiveConfig()
{
    XORTutorial::TutorialConfig config;
    config.interactive = false;
    config.show_matrices = false;
    config.show_calculations = false;
    config.step_by_step = false;
    config.detail_level = TutorialLevel::Basic;
    return config;
}

QTEST_MAIN(TestXORTutorial)
#include "test_xor_tutorial.moc"