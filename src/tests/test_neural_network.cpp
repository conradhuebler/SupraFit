/*
 * Neural Network Unit Tests
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
#include <QJsonDocument>
#include <QTemporaryFile>

#include "../ml/neural_network.h"
#include "../ml/tutorial/examples/xor_tutorial.h"

/**
 * @brief Comprehensive test suite for Neural Network implementation
 * 
 * Tests all critical aspects of the educational neural network library:
 * - Network construction and architecture validation
 * - Forward propagation correctness
 * - XOR problem solution validation
 * - Model persistence (save/load)
 * - Error handling and edge cases
 * - Tutorial functionality
 * 
 * Claude Generated - 2025
 */
class TestNeuralNetwork : public QObject
{
    Q_OBJECT

private slots:
    void testNetworkConstruction();
    void testNetworkArchitecture();
    void testForwardPropagation();
    void testXORSolution();
    void testModelPersistence();
    void testErrorHandling();
    void testTutorialMode();
    void testPerformanceMetrics();
    void testActivationFunctions();
    void testLayerOperations();

private:
    // Helper functions
    bool isApproximatelyEqual(double a, double b, double tolerance = 1e-6);
    bool isVectorEqual(const Eigen::VectorXd& v1, const Eigen::VectorXd& v2, double tolerance = 1e-6);
};

void TestNeuralNetwork::testNetworkConstruction()
{
    // Test default constructor
    NeuralNetwork empty_network;
    QCOMPARE(empty_network.layerCount(), static_cast<size_t>(0));
    QCOMPARE(empty_network.getTotalParameters(), static_cast<size_t>(0));
    
    // Test architecture constructor
    std::vector<int> architecture = {3, 5, 2};
    std::vector<QString> activations = {"relu", "sigmoid"};
    
    NeuralNetwork network(architecture, activations);
    QCOMPARE(network.layerCount(), static_cast<size_t>(2));
    
    auto arch = network.getArchitecture();
    QCOMPARE(arch.size(), static_cast<size_t>(3));
    QCOMPARE(arch[0], 3);
    QCOMPARE(arch[1], 5);
    QCOMPARE(arch[2], 2);
    
    // Test parameter count: (3*5 + 5) + (5*2 + 2) = 15 + 5 + 10 + 2 = 32
    QCOMPARE(network.getTotalParameters(), static_cast<size_t>(32));
}

void TestNeuralNetwork::testNetworkArchitecture()
{
    NeuralNetwork network;
    
    // Add layers manually
    network.addLayer(4, 6, "relu");
    network.addLayer(6, 3, "tanh");
    network.addLayer(3, 1, "sigmoid");
    
    QCOMPARE(network.layerCount(), static_cast<size_t>(3));
    
    auto arch = network.getArchitecture();
    QVERIFY(arch.size() == 4);
    QCOMPARE(arch[0], 4);  // Input
    QCOMPARE(arch[1], 6);  // Hidden 1
    QCOMPARE(arch[2], 3);  // Hidden 2
    QCOMPARE(arch[3], 1);  // Output
    
    QVERIFY(network.validateArchitecture());
}

void TestNeuralNetwork::testForwardPropagation()
{
    // Simple 2-2-1 network
    NeuralNetwork network({2, 2, 1}, {"relu", "linear"});
    
    // Test with known input
    Eigen::VectorXd input(2);
    input << 1.0, 0.5;
    
    Eigen::VectorXd output = network.predict(input);
    
    QVERIFY(!output.isZero());
    QCOMPARE(output.size(), 1);
    QCOMPARE(network.getLastError(), NeuralNetwork::ErrorCode::Success);
    
    // Test invalid input size
    Eigen::VectorXd bad_input(3);
    bad_input << 1.0, 0.5, 0.25;
    
    Eigen::VectorXd bad_output = network.predict(bad_input);
    QVERIFY(bad_output.size() == 0);
    QVERIFY(network.getLastError() != NeuralNetwork::ErrorCode::Success);
}

void TestNeuralNetwork::testXORSolution()
{
    XORTutorial::TutorialConfig config;
    config.interactive = false;
    config.show_calculations = false;
    
    XORTutorial tutorial(config);
    
    // Test that XOR solution is correct
    QVERIFY(tutorial.validateXORSolution());
    
    // Test individual XOR cases
    NeuralNetwork network = tutorial.createPretrainedXORNetwork();
    
    // Test XOR truth table
    Eigen::VectorXd input1(2); input1 << 0, 0;
    Eigen::VectorXd input2(2); input2 << 0, 1;
    Eigen::VectorXd input3(2); input3 << 1, 0;
    Eigen::VectorXd input4(2); input4 << 1, 1;
    
    double output1 = network.predict(input1)(0);
    double output2 = network.predict(input2)(0);
    double output3 = network.predict(input3)(0);
    double output4 = network.predict(input4)(0);
    
    // XOR: 0 XOR 0 = 0, 0 XOR 1 = 1, 1 XOR 0 = 1, 1 XOR 1 = 0
    QVERIFY(output1 < 0.5);  // Should be close to 0
    QVERIFY(output2 > 0.5);  // Should be close to 1
    QVERIFY(output3 > 0.5);  // Should be close to 1
    QVERIFY(output4 < 0.5);  // Should be close to 0
}

void TestNeuralNetwork::testModelPersistence()
{
    // Create and configure network
    NeuralNetwork original({3, 4, 2}, {"relu", "softmax"});
    
    // Save to JSON
    QJsonObject saved_model = original.saveModel();
    QVERIFY(!saved_model.isEmpty());
    QCOMPARE(saved_model["type"].toString(), "suprafit_neural_network");
    
    // Load into new network
    NeuralNetwork loaded;
    QVERIFY(loaded.loadModel(saved_model));
    
    // Compare architectures
    auto orig_arch = original.getArchitecture();
    auto loaded_arch = loaded.getArchitecture();
    QCOMPARE(orig_arch.size(), loaded_arch.size());
    
    for (size_t i = 0; i < orig_arch.size(); ++i) {
        QCOMPARE(orig_arch[i], loaded_arch[i]);
    }
    
    // Test file save/load
    QTemporaryFile temp_file;
    QVERIFY(temp_file.open());
    QString filename = temp_file.fileName();
    temp_file.close();
    
    QVERIFY(original.saveToFile(filename));
    
    NeuralNetwork from_file;
    QVERIFY(from_file.loadFromFile(filename));
    
    // Compare with original
    auto file_arch = from_file.getArchitecture();
    QCOMPARE(orig_arch.size(), file_arch.size());
}

void TestNeuralNetwork::testErrorHandling()
{
    NeuralNetwork network;
    
    // Test prediction on empty network
    Eigen::VectorXd input(2);
    input << 1.0, 2.0;
    
    Eigen::VectorXd output = network.predict(input);
    QVERIFY(output.size() == 0);
    QCOMPARE(network.getLastError(), NeuralNetwork::ErrorCode::EmptyNetwork);
    
    // Test invalid layer access
    auto& layer = network.getLayer(999);
    QCOMPARE(network.getLastError(), NeuralNetwork::ErrorCode::InvalidInput);
    
    // Test invalid JSON loading
    QJsonObject bad_json;
    bad_json["type"] = "invalid_type";
    
    QVERIFY(!network.loadModel(bad_json));
    QCOMPARE(network.getLastError(), NeuralNetwork::ErrorCode::JsonError);
}

void TestNeuralNetwork::testTutorialMode()
{
    NeuralNetwork network({2, 2, 1}, {"relu", "sigmoid"});
    
    // Enable tutorial mode
    network.enableTutorialMode(TutorialLevel::Basic, false);
    QVERIFY(network.isTutorialEnabled());
    
    // Test prediction with tutorial (should not crash)
    Eigen::VectorXd input(2);
    input << 0.5, 0.3;
    
    Eigen::VectorXd output = network.predict(input);
    QVERIFY(!output.isZero());
    QCOMPARE(network.getLastError(), NeuralNetwork::ErrorCode::Success);
    
    // Disable tutorial mode
    network.disableTutorialMode();
    QVERIFY(!network.isTutorialEnabled());
}

void TestNeuralNetwork::testPerformanceMetrics()
{
    NeuralNetwork network({10, 8, 4}, {"relu", "softmax"});
    
    Eigen::VectorXd input = Eigen::VectorXd::Random(10);
    network.predict(input);
    
    auto metrics = network.getPerformanceMetrics();
    
    // Check that metrics are populated
    QVERIFY(metrics.total_parameters > 0);
    QVERIFY(metrics.memory_usage_mb > 0);
    QVERIFY(metrics.inference_time_ms >= 0);
    
    // Test memory calculation
    double expected_mb = (network.getTotalParameters() * sizeof(double)) / (1024.0 * 1024.0);
    QVERIFY(isApproximatelyEqual(network.getMemoryUsageMB(), expected_mb, 0.001));
}

void TestNeuralNetwork::testActivationFunctions()
{
    // Test different activation functions
    std::vector<QString> activations = {"relu", "sigmoid", "tanh", "linear", "softmax"};
    
    for (const QString& activation : activations) {
        NeuralNetwork network({3, 2}, {activation});
        
        Eigen::VectorXd input = Eigen::VectorXd::Random(3);
        Eigen::VectorXd output = network.predict(input);
        
        QVERIFY(!output.isZero() || activation == "linear");
        QCOMPARE(output.size(), 2);
        QCOMPARE(network.getLastError(), NeuralNetwork::ErrorCode::Success);
    }
}

void TestNeuralNetwork::testLayerOperations()
{
    NeuralNetwork network;
    
    // Test layer addition
    network.addLayer(5, 3, "relu");
    network.addLayer(3, 1, "sigmoid");
    
    QCOMPARE(network.layerCount(), static_cast<size_t>(2));
    
    // Test layer access
    const auto& layer1 = network.getLayer(0);
    QCOMPARE(layer1.inputSize(), 5);
    QCOMPARE(layer1.outputSize(), 3);
    
    // Test clear layers
    network.clearLayers();
    QCOMPARE(network.layerCount(), static_cast<size_t>(0));
}

bool TestNeuralNetwork::isApproximatelyEqual(double a, double b, double tolerance)
{
    return std::abs(a - b) < tolerance;
}

bool TestNeuralNetwork::isVectorEqual(const Eigen::VectorXd& v1, const Eigen::VectorXd& v2, double tolerance)
{
    if (v1.size() != v2.size()) return false;
    
    for (int i = 0; i < v1.size(); ++i) {
        if (!isApproximatelyEqual(v1(i), v2(i), tolerance)) {
            return false;
        }
    }
    return true;
}

QTEST_MAIN(TestNeuralNetwork)
#include "test_neural_network.moc"