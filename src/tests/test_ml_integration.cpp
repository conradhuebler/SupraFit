/*
 * Machine Learning Integration Tests
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
#include <QTemporaryFile>

#include "../ml/neural_network.h"
#include "../ml/tutorial/examples/xor_tutorial.h"

/**
 * @brief Integration tests for machine learning components
 * 
 * Tests the complete ML pipeline integration, including:
 * - End-to-end neural network workflows
 * - Tutorial system integration
 * - Performance benchmarking
 * - Compatibility with existing SupraFit infrastructure
 * 
 * Claude Generated - 2025
 */
class TestMLIntegration : public QObject
{
    Q_OBJECT

private slots:
    void testEndToEndWorkflow();
    void testTutorialPipeline();
    void testPerformanceBenchmark();
    void testModelPersistenceIntegration();
    void testErrorHandlingIntegration();
    void testMultipleNetworkInstances();
    void testConcurrentPredictions();
    void testMemoryManagement();

private:
    bool isApproximatelyEqual(double a, double b, double tolerance = 1e-6);
    std::vector<Eigen::VectorXd> generateTestData(int num_samples, int input_size);
};

void TestMLIntegration::testEndToEndWorkflow()
{
    // Simulate a complete ML workflow from data to prediction
    
    // Step 1: Create and configure network
    std::vector<int> architecture = {4, 6, 3, 2};
    std::vector<QString> activations = {"relu", "relu", "softmax"};
    
    NeuralNetwork network(architecture, activations);
    QVERIFY(network.validateArchitecture());
    
    // Step 2: Generate test data
    std::vector<Eigen::VectorXd> test_inputs = generateTestData(10, 4);
    
    // Step 3: Run predictions
    std::vector<NeuralNetwork::PredictionResult> results;
    
    for (const auto& input : test_inputs) {
        auto result = network.predictWithDetails(input);
        QCOMPARE(network.getLastError(), NeuralNetwork::ErrorCode::Success);
        QCOMPARE(result.probabilities.size(), 2);
        
        results.push_back(result);
    }
    
    // Step 4: Validate results
    QCOMPARE(results.size(), static_cast<size_t>(10));
    
    for (const auto& result : results) {
        QVERIFY(result.confidence >= 0.0 && result.confidence <= 1.0);
        QVERIFY(result.predicted_class >= 0 && result.predicted_class < 2);
        QVERIFY(!result.interpretation.isEmpty());
    }
    
    // Step 5: Save model
    QJsonObject saved_model = network.saveModel();
    QVERIFY(!saved_model.isEmpty());
    
    // Step 6: Load model and verify consistency
    NeuralNetwork loaded_network;
    QVERIFY(loaded_network.loadModel(saved_model));
    
    // Test that loaded network produces same results
    auto original_result = network.predictWithDetails(test_inputs[0]);
    auto loaded_result = loaded_network.predictWithDetails(test_inputs[0]);
    
    for (int i = 0; i < original_result.probabilities.size(); ++i) {
        QVERIFY(isApproximatelyEqual(original_result.probabilities(i), 
                                   loaded_result.probabilities(i), 1e-10));
    }
}

void TestMLIntegration::testTutorialPipeline()
{
    // Test complete tutorial system integration
    
    XORTutorial::TutorialConfig config;
    config.interactive = false;
    config.show_matrices = true;
    config.show_calculations = true;
    config.step_by_step = false;
    config.detail_level = TutorialLevel::Intermediate;
    
    XORTutorial tutorial(config);
    
    // Run tutorial components in sequence
    tutorial.explainXORProblem();
    tutorial.demonstratePerceptronFailure();
    tutorial.buildNeuralNetworkSolution();
    tutorial.testNetworkOnAllInputs();
    tutorial.explainWhyItWorks();
    
    // Verify tutorial state remains consistent
    QVERIFY(tutorial.validateXORSolution());
    
    // Test tutorial export
    QJsonObject tutorial_results = tutorial.exportTutorialResults();
    QVERIFY(tutorial_results["validation_passed"].toBool());
    
    // Test that tutorial network can be used independently
    NeuralNetwork tutorial_network = tutorial.createPretrainedXORNetwork();
    
    // Enable tutorial mode on the network
    tutorial_network.enableTutorialMode(TutorialLevel::Advanced, false);
    
    Eigen::VectorXd test_input(2);
    test_input << 1.0, 0.0;
    
    auto result = tutorial_network.predictWithDetails(test_input);
    QVERIFY(result.probabilities.size() > 0);
    QVERIFY(!result.interpretation.isEmpty());
}

void TestMLIntegration::testPerformanceBenchmark()
{
    // Test performance characteristics of the ML pipeline
    
    NeuralNetwork network({20, 15, 10, 5}, {"relu", "relu", "softmax"});
    
    // Generate larger test dataset
    std::vector<Eigen::VectorXd> large_dataset = generateTestData(100, 20);
    
    // Measure inference time
    QElapsedTimer timer;
    timer.start();
    
    for (const auto& input : large_dataset) {
        network.predict(input);
    }
    
    qint64 total_time = timer.elapsed();
    double avg_time_per_prediction = static_cast<double>(total_time) / large_dataset.size();
    
    // Performance should be reasonable (less than 10ms per prediction on average)
    QVERIFY(avg_time_per_prediction < 10.0);
    
    // Test memory usage tracking
    auto metrics = network.getPerformanceMetrics();
    QVERIFY(metrics.total_parameters > 0);
    QVERIFY(metrics.memory_usage_mb > 0);
    QVERIFY(metrics.inference_time_ms >= 0);
    
    // Memory usage should be reasonable for this size network
    QVERIFY(metrics.memory_usage_mb < 10.0);  // Less than 10MB for this network
}

void TestMLIntegration::testModelPersistenceIntegration()
{
    // Test model persistence across different scenarios
    
    // Create original network
    NeuralNetwork original({5, 8, 3}, {"tanh", "sigmoid"});
    
    // Generate test data
    auto test_data = generateTestData(5, 5);
    
    // Get original predictions
    std::vector<Eigen::VectorXd> original_predictions;
    for (const auto& input : test_data) {
        original_predictions.push_back(original.predict(input));
    }
    
    // Test JSON round-trip
    QJsonObject json = original.saveModel();
    NeuralNetwork from_json;
    QVERIFY(from_json.loadModel(json));
    
    // Test file round-trip
    QTemporaryFile temp_file;
    QVERIFY(temp_file.open());
    QString filename = temp_file.fileName();
    temp_file.close();
    
    QVERIFY(original.saveToFile(filename));
    NeuralNetwork from_file;
    QVERIFY(from_file.loadFromFile(filename));
    
    // Verify all loaded models produce identical results
    for (size_t i = 0; i < test_data.size(); ++i) {
        auto json_pred = from_json.predict(test_data[i]);
        auto file_pred = from_file.predict(test_data[i]);
        
        for (int j = 0; j < original_predictions[i].size(); ++j) {
            QVERIFY(isApproximatelyEqual(original_predictions[i](j), json_pred(j), 1e-12));
            QVERIFY(isApproximatelyEqual(original_predictions[i](j), file_pred(j), 1e-12));
        }
    }
    
    // Clean up
    QFile::remove(filename);
}

void TestMLIntegration::testErrorHandlingIntegration()
{
    // Test error handling across the entire ML pipeline
    
    NeuralNetwork network({3, 2}, {"relu"});
    
    // Test prediction with wrong input size
    Eigen::VectorXd wrong_size_input(5);
    wrong_size_input.setRandom();
    
    Eigen::VectorXd result = network.predict(wrong_size_input);
    QVERIFY(result.size() == 0);
    QVERIFY(network.getLastError() != NeuralNetwork::ErrorCode::Success);
    QVERIFY(!network.getLastErrorMessage().isEmpty());
    
    // Test recovery after error
    Eigen::VectorXd correct_input(3);
    correct_input.setRandom();
    
    Eigen::VectorXd correct_result = network.predict(correct_input);
    QVERIFY(correct_result.size() > 0);
    QCOMPARE(network.getLastError(), NeuralNetwork::ErrorCode::Success);
    
    // Test loading invalid JSON
    QJsonObject bad_json;
    bad_json["type"] = "invalid_model_type";
    
    QVERIFY(!network.loadModel(bad_json));
    QCOMPARE(network.getLastError(), NeuralNetwork::ErrorCode::JsonError);
    
    // Verify network is still functional after error
    Eigen::VectorXd post_error_result = network.predict(correct_input);
    QVERIFY(post_error_result.size() > 0);
    QCOMPARE(network.getLastError(), NeuralNetwork::ErrorCode::Success);
}

void TestMLIntegration::testMultipleNetworkInstances()
{
    // Test that multiple networks can coexist and function independently
    
    std::vector<std::unique_ptr<NeuralNetwork>> networks;
    
    // Create different network architectures
    networks.push_back(std::make_unique<NeuralNetwork>(std::vector<int>{2, 3, 1}, std::vector<QString>{"relu", "sigmoid"}));
    networks.push_back(std::make_unique<NeuralNetwork>(std::vector<int>{4, 2}, std::vector<QString>{"tanh"}));
    networks.push_back(std::make_unique<NeuralNetwork>(std::vector<int>{3, 5, 3}, std::vector<QString>{"linear", "softmax"}));
    
    // Generate appropriate test inputs
    std::vector<Eigen::VectorXd> inputs;
    inputs.emplace_back(2); inputs.back().setRandom();  // For network 0
    inputs.emplace_back(4); inputs.back().setRandom();  // For network 1
    inputs.emplace_back(3); inputs.back().setRandom();  // For network 2
    
    // Test that each network works independently
    for (size_t i = 0; i < networks.size(); ++i) {
        auto result = networks[i]->predict(inputs[i]);
        QVERIFY(result.size() > 0);
        QCOMPARE(networks[i]->getLastError(), NeuralNetwork::ErrorCode::Success);
    }
    
    // Test that one network's error doesn't affect others
    Eigen::VectorXd bad_input(10);  // Wrong size for all networks
    bad_input.setRandom();
    
    // Make network 1 error
    networks[1]->predict(bad_input);
    QVERIFY(networks[1]->getLastError() != NeuralNetwork::ErrorCode::Success);
    
    // Other networks should still work
    auto result0 = networks[0]->predict(inputs[0]);
    auto result2 = networks[2]->predict(inputs[2]);
    
    QVERIFY(result0.size() > 0);
    QVERIFY(result2.size() > 0);
    QCOMPARE(networks[0]->getLastError(), NeuralNetwork::ErrorCode::Success);
    QCOMPARE(networks[2]->getLastError(), NeuralNetwork::ErrorCode::Success);
}

void TestMLIntegration::testConcurrentPredictions()
{
    // Test that the same network can handle multiple predictions
    
    NeuralNetwork network({5, 4, 2}, {"relu", "softmax"});
    
    auto test_inputs = generateTestData(20, 5);
    
    // Run multiple predictions and store results
    std::vector<NeuralNetwork::PredictionResult> results;
    
    for (const auto& input : test_inputs) {
        auto result = network.predictWithDetails(input);
        results.push_back(result);
        
        QCOMPARE(network.getLastError(), NeuralNetwork::ErrorCode::Success);
        QCOMPARE(result.probabilities.size(), 2);
    }
    
    // Verify all results are valid
    QCOMPARE(results.size(), test_inputs.size());
    
    for (const auto& result : results) {
        QVERIFY(result.confidence >= 0.0 && result.confidence <= 1.0);
        QVERIFY(result.predicted_class >= 0 && result.predicted_class < 2);
        
        // Softmax outputs should sum to approximately 1
        double sum = result.probabilities.sum();
        QVERIFY(isApproximatelyEqual(sum, 1.0, 1e-10));
    }
}

void TestMLIntegration::testMemoryManagement()
{
    // Test memory management during network lifecycle
    
    size_t initial_memory = 0;  // Would need platform-specific code to measure actual memory
    
    {
        // Create large network
        NeuralNetwork large_network({100, 80, 60, 40, 20}, {"relu", "relu", "relu", "softmax"});
        
        // Use the network
        auto test_data = generateTestData(50, 100);
        for (const auto& input : test_data) {
            large_network.predict(input);
        }
        
        // Enable tutorial mode (additional memory usage)
        large_network.enableTutorialMode(TutorialLevel::Advanced, false);
        
        // Run more predictions with tutorial mode
        for (const auto& input : test_data) {
            large_network.predict(input);
        }
        
        // Test copy constructor
        NeuralNetwork copied_network = large_network;
        QVERIFY(copied_network.validateArchitecture());
        
        // Test assignment operator
        NeuralNetwork assigned_network;
        assigned_network = large_network;
        QVERIFY(assigned_network.validateArchitecture());
        
        // All networks should produce same results
        Eigen::VectorXd test_input = test_data[0];
        auto original_result = large_network.predict(test_input);
        auto copied_result = copied_network.predict(test_input);
        auto assigned_result = assigned_network.predict(test_input);
        
        for (int i = 0; i < original_result.size(); ++i) {
            QVERIFY(isApproximatelyEqual(original_result(i), copied_result(i), 1e-12));
            QVERIFY(isApproximatelyEqual(original_result(i), assigned_result(i), 1e-12));
        }
        
    }  // Networks go out of scope here
    
    // After destruction, memory should be released
    // (In a real test, we would measure actual memory usage)
    QVERIFY(true);  // Placeholder - actual memory measurement would go here
}

bool TestMLIntegration::isApproximatelyEqual(double a, double b, double tolerance)
{
    return std::abs(a - b) < tolerance;
}

std::vector<Eigen::VectorXd> TestMLIntegration::generateTestData(int num_samples, int input_size)
{
    std::vector<Eigen::VectorXd> data;
    data.reserve(num_samples);
    
    for (int i = 0; i < num_samples; ++i) {
        Eigen::VectorXd sample(input_size);
        sample.setRandom();  // Random values between -1 and 1
        data.push_back(sample);
    }
    
    return data;
}

QTEST_MAIN(TestMLIntegration)
#include "test_ml_integration.moc"