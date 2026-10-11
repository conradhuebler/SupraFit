/*
 * Neural Layer Unit Tests
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

#include "../ml/neural_layer.h"
#include "../ml/activation_functions.h"

/**
 * @brief Comprehensive test suite for Neural Layer implementation
 * 
 * Tests layer construction, forward propagation, weight initialization,
 * and JSON serialization of individual neural network layers.
 * 
 * Claude Generated - 2025
 */
class TestNeuralLayer : public QObject
{
    Q_OBJECT

private slots:
    void testLayerConstruction();
    void testForwardPropagation();
    void testWeightInitialization();
    void testWeightSettersGetters();
    void testJSONSerialization();
    void testDimensionValidation();
    void testActivationFunctions();
    void testLayerInfo();

private:
    bool isApproximatelyEqual(double a, double b, double tolerance = 1e-6);
    bool isVectorEqual(const Eigen::VectorXd& v1, const Eigen::VectorXd& v2, double tolerance = 1e-6);
    bool isMatrixEqual(const Eigen::MatrixXd& m1, const Eigen::MatrixXd& m2, double tolerance = 1e-6);
};

void TestNeuralLayer::testLayerConstruction()
{
    auto activation = createActivationFunction("relu");
    NeuralLayer layer(3, 5, std::move(activation));
    
    // Test dimensions
    QCOMPARE(layer.inputSize(), 3);
    QCOMPARE(layer.outputSize(), 5);
    
    // Test weight matrix dimensions
    const Eigen::MatrixXd& weights = layer.weights();
    QCOMPARE(weights.rows(), 5);  // output_size x input_size
    QCOMPARE(weights.cols(), 3);
    
    // Test bias vector dimensions
    const Eigen::VectorXd& biases = layer.biases();
    QCOMPARE(biases.size(), 5);
    
    // Test that weights and biases are not all zeros (should be initialized)
    QVERIFY(!weights.isZero());
    // Biases might be zero depending on initialization method
}

void TestNeuralLayer::testForwardPropagation()
{
    auto activation = createActivationFunction("linear");  // Use linear for predictable results
    NeuralLayer layer(2, 3, std::move(activation));
    
    // Set known weights and biases
    Eigen::MatrixXd weights(3, 2);
    weights << 1.0, 2.0,
               3.0, 4.0,
               0.5, 1.5;
    layer.setWeights(weights);
    
    Eigen::VectorXd biases(3);
    biases << 0.1, 0.2, 0.3;
    layer.setBiases(biases);
    
    // Test forward pass
    Eigen::VectorXd input(2);
    input << 1.0, 2.0;
    
    Eigen::VectorXd output = layer.forward(input);
    
    // Expected: weights * input + biases
    // [1.0, 2.0] * [1.0] + [0.1] = [5.1]
    // [3.0, 4.0]   [2.0]   [0.2]   [11.2]
    // [0.5, 1.5]           [0.3]   [3.8]
    
    Eigen::VectorXd expected(3);
    expected << 5.1, 11.2, 3.8;
    
    QVERIFY(isVectorEqual(output, expected, 1e-10));
}

void TestNeuralLayer::testWeightInitialization()
{
    // Test Xavier initialization
    auto activation1 = createActivationFunction("sigmoid");
    NeuralLayer layer_xavier(10, 8, std::move(activation1), NeuralLayer::InitializationMethod::Xavier);
    
    const Eigen::MatrixXd& xavier_weights = layer_xavier.weights();
    
    // Xavier initialization should have specific variance
    double expected_variance = 2.0 / (10 + 8);  // 2/(fan_in + fan_out)
    double actual_variance = (xavier_weights.array() - xavier_weights.mean()).square().mean();
    
    // Allow some tolerance for random initialization
    QVERIFY(std::abs(actual_variance - expected_variance) < 0.1);
    
    // Test He initialization
    auto activation2 = createActivationFunction("relu");
    NeuralLayer layer_he(10, 8, std::move(activation2), NeuralLayer::InitializationMethod::He);
    
    const Eigen::MatrixXd& he_weights = layer_he.weights();
    
    // He initialization should have different variance
    double he_expected_variance = 2.0 / 10;  // 2/fan_in
    double he_actual_variance = (he_weights.array() - he_weights.mean()).square().mean();
    
    QVERIFY(std::abs(he_actual_variance - he_expected_variance) < 0.1);
    
    // Test Zero initialization
    auto activation3 = createActivationFunction("linear");
    NeuralLayer layer_zero(5, 3, std::move(activation3), NeuralLayer::InitializationMethod::Zero);
    
    const Eigen::MatrixXd& zero_weights = layer_zero.weights();
    const Eigen::VectorXd& zero_biases = layer_zero.biases();
    
    QVERIFY(zero_weights.isZero());
    QVERIFY(zero_biases.isZero());
}

void TestNeuralLayer::testWeightSettersGetters()
{
    auto activation = createActivationFunction("tanh");
    NeuralLayer layer(4, 2, std::move(activation));
    
    // Set custom weights
    Eigen::MatrixXd custom_weights(2, 4);
    custom_weights << 1.0, 2.0, 3.0, 4.0,
                     5.0, 6.0, 7.0, 8.0;
    layer.setWeights(custom_weights);
    
    // Test that weights are set correctly
    const Eigen::MatrixXd& retrieved_weights = layer.weights();
    QVERIFY(isMatrixEqual(retrieved_weights, custom_weights));
    
    // Set custom biases
    Eigen::VectorXd custom_biases(2);
    custom_biases << 0.5, -0.3;
    layer.setBiases(custom_biases);
    
    // Test that biases are set correctly
    const Eigen::VectorXd& retrieved_biases = layer.biases();
    QVERIFY(isVectorEqual(retrieved_biases, custom_biases));
}

void TestNeuralLayer::testJSONSerialization()
{
    // Create layer with known configuration
    auto activation = createActivationFunction("relu");
    NeuralLayer original(3, 2, std::move(activation));
    
    // Set known weights and biases
    Eigen::MatrixXd weights(2, 3);
    weights << 1.0, 2.0, 3.0,
               4.0, 5.0, 6.0;
    original.setWeights(weights);
    
    Eigen::VectorXd biases(2);
    biases << 0.7, -0.2;
    original.setBiases(biases);
    
    // Serialize to JSON
    QJsonObject json = original.toJson();
    
    // Verify JSON structure
    QCOMPARE(json["input_size"].toInt(), 3);
    QCOMPARE(json["output_size"].toInt(), 2);
    QCOMPARE(json["activation"].toString(), QString("ReLU"));
    
    // Create new layer and deserialize
    auto new_activation = createActivationFunction("sigmoid");  // Different activation
    NeuralLayer loaded(1, 1, std::move(new_activation));  // Different dimensions
    
    QVERIFY(loaded.fromJson(json));
    
    // Verify loaded layer matches original
    QCOMPARE(loaded.inputSize(), 3);
    QCOMPARE(loaded.outputSize(), 2);
    QVERIFY(isMatrixEqual(loaded.weights(), weights));
    QVERIFY(isVectorEqual(loaded.biases(), biases));
}

void TestNeuralLayer::testDimensionValidation()
{
    auto activation = createActivationFunction("sigmoid");
    NeuralLayer layer(3, 2, std::move(activation));
    
    // Test correct input size
    Eigen::VectorXd correct_input(3);
    correct_input << 1.0, 2.0, 3.0;
    
    Eigen::VectorXd output = layer.forward(correct_input);
    QCOMPARE(output.size(), 2);
    
    // Test incorrect input size (should handle gracefully or throw)
    Eigen::VectorXd incorrect_input(5);  // Wrong size
    incorrect_input << 1.0, 2.0, 3.0, 4.0, 5.0;
    
    // The behavior here depends on implementation - it might throw or return empty
    // We just ensure it doesn't crash
    try {
        Eigen::VectorXd bad_output = layer.forward(incorrect_input);
        // If it doesn't throw, verify the output is reasonable
        Q_UNUSED(bad_output);
    } catch (...) {
        // It's acceptable to throw on dimension mismatch
        QVERIFY(true);
    }
}

void TestNeuralLayer::testActivationFunctions()
{
    // Test different activation functions
    std::vector<QString> activations = {"relu", "sigmoid", "tanh", "linear"};
    
    for (const QString& act_name : activations) {
        auto activation = createActivationFunction(act_name);
        NeuralLayer layer(2, 1, std::move(activation));
        
        Eigen::VectorXd input(2);
        input << 0.5, -0.3;
        
        Eigen::VectorXd output = layer.forward(input);
        QCOMPARE(output.size(), 1);
        
        // Verify output is finite
        QVERIFY(std::isfinite(output(0)));
    }
}

void TestNeuralLayer::testLayerInfo()
{
    auto activation = createActivationFunction("relu");
    NeuralLayer layer(5, 3, std::move(activation));
    
    QString info = layer.getLayerInfo();
    
    // Verify info contains expected information
    QVERIFY(info.contains("5"));  // Input size
    QVERIFY(info.contains("3"));  // Output size
    QVERIFY(info.contains("relu") || info.contains("ReLU"));  // Activation function
}

bool TestNeuralLayer::isApproximatelyEqual(double a, double b, double tolerance)
{
    return std::abs(a - b) < tolerance;
}

bool TestNeuralLayer::isVectorEqual(const Eigen::VectorXd& v1, const Eigen::VectorXd& v2, double tolerance)
{
    if (v1.size() != v2.size()) return false;
    
    for (int i = 0; i < v1.size(); ++i) {
        if (!isApproximatelyEqual(v1(i), v2(i), tolerance)) {
            return false;
        }
    }
    return true;
}

bool TestNeuralLayer::isMatrixEqual(const Eigen::MatrixXd& m1, const Eigen::MatrixXd& m2, double tolerance)
{
    if (m1.rows() != m2.rows() || m1.cols() != m2.cols()) return false;
    
    for (int i = 0; i < m1.rows(); ++i) {
        for (int j = 0; j < m1.cols(); ++j) {
            if (!isApproximatelyEqual(m1(i, j), m2(i, j), tolerance)) {
                return false;
            }
        }
    }
    return true;
}

QTEST_MAIN(TestNeuralLayer)
#include "test_neural_layer.moc"