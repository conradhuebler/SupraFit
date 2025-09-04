/*
 * Activation Functions Unit Tests
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
#include <cmath>

#include "../ml/activation_functions.h"

/**
 * @brief Comprehensive test suite for activation functions
 * 
 * Tests mathematical correctness, edge cases, and educational properties
 * of all implemented activation functions used in the neural network library.
 * 
 * Claude Generated - 2025
 */
class TestActivationFunctions : public QObject
{
    Q_OBJECT

private slots:
    void testReLUFunction();
    void testSigmoidFunction();
    void testTanhFunction();
    void testLinearFunction();
    void testSoftmaxFunction();
    void testActivationFactory();
    void testEdgeCases();
    void testDerivatives();

private:
    bool isApproximatelyEqual(double a, double b, double tolerance = 1e-6);
    bool isVectorEqual(const Eigen::VectorXd& v1, const Eigen::VectorXd& v2, double tolerance = 1e-6);
};

void TestActivationFunctions::testReLUFunction()
{
    ReLUActivation relu;
    
    // Test positive values
    QCOMPARE(relu.forward(2.5), 2.5);
    QCOMPARE(relu.forward(0.1), 0.1);
    QCOMPARE(relu.forward(100.0), 100.0);
    
    // Test negative values
    QCOMPARE(relu.forward(-1.5), 0.0);
    QCOMPARE(relu.forward(-0.001), 0.0);
    QCOMPARE(relu.forward(-100.0), 0.0);
    
    // Test zero
    QCOMPARE(relu.forward(0.0), 0.0);
    
    // Test vector input
    Eigen::VectorXd input(4);
    input << -2.0, -0.5, 0.0, 1.5;
    
    Eigen::VectorXd expected(4);
    expected << 0.0, 0.0, 0.0, 1.5;
    
    Eigen::VectorXd result = relu.forwardVector(input);
    QVERIFY(isVectorEqual(result, expected));
    
    // Test derivative
    QCOMPARE(relu.backward(2.5), 1.0);  // x > 0
    QCOMPARE(relu.backward(-1.0), 0.0);  // x < 0
    QCOMPARE(relu.backward(0.0), 0.0);   // x = 0 (defined as 0)
}

void TestActivationFunctions::testSigmoidFunction()
{
    SigmoidActivation sigmoid;
    
    // Test known values
    QVERIFY(isApproximatelyEqual(sigmoid.forward(0.0), 0.5, 1e-6));
    
    // Test asymptotic behavior
    QVERIFY(sigmoid.forward(10.0) > 0.99);   // Should approach 1
    QVERIFY(sigmoid.forward(-10.0) < 0.01);  // Should approach 0
    
    // Test that output is always between 0 and 1
    for (double x = -10.0; x <= 10.0; x += 0.5) {
        double result = sigmoid.forward(x);
        QVERIFY(result >= 0.0 && result <= 1.0);
    }
    
    // Test vector input
    Eigen::VectorXd input(3);
    input << 0.0, -2.0, 2.0;
    
    Eigen::VectorXd result = sigmoid.forwardVector(input);
    
    QVERIFY(isApproximatelyEqual(result(0), 0.5, 1e-6));
    QVERIFY(result(1) < 0.2);  // sigmoid(-2) should be small
    QVERIFY(result(2) > 0.8);  // sigmoid(2) should be large
    
    // Test derivative (should be s(x) * (1 - s(x)))
    double x = 1.0;
    double s_x = sigmoid.forward(x);
    double expected_derivative = s_x * (1.0 - s_x);
    QVERIFY(isApproximatelyEqual(sigmoid.backward(x), expected_derivative, 1e-6));
}

void TestActivationFunctions::testTanhFunction()
{
    TanhActivation tanh_func;
    
    // Test known values
    QVERIFY(isApproximatelyEqual(tanh_func.forward(0.0), 0.0, 1e-6));
    
    // Test asymptotic behavior
    QVERIFY(tanh_func.forward(10.0) > 0.99);   // Should approach 1
    QVERIFY(tanh_func.forward(-10.0) < -0.99); // Should approach -1
    
    // Test that output is always between -1 and 1
    for (double x = -5.0; x <= 5.0; x += 0.2) {
        double result = tanh_func.forward(x);
        QVERIFY(result >= -1.0 && result <= 1.0);
    }
    
    // Test symmetry: tanh(-x) = -tanh(x)
    for (double x = 0.1; x <= 3.0; x += 0.3) {
        QVERIFY(isApproximatelyEqual(tanh_func.forward(-x), -tanh_func.forward(x), 1e-10));
    }
    
    // Test derivative (should be 1 - tanh²(x))
    double x = 0.5;
    double tanh_x = tanh_func.forward(x);
    double expected_derivative = 1.0 - tanh_x * tanh_x;
    QVERIFY(isApproximatelyEqual(tanh_func.backward(x), expected_derivative, 1e-6));
}

void TestActivationFunctions::testLinearFunction()
{
    LinearActivation linear;
    
    // Test that output equals input
    QCOMPARE(linear.forward(0.0), 0.0);
    QCOMPARE(linear.forward(2.5), 2.5);
    QCOMPARE(linear.forward(-1.8), -1.8);
    QCOMPARE(linear.forward(100.0), 100.0);
    
    // Test vector input
    Eigen::VectorXd input(4);
    input << -2.3, 0.0, 1.7, 42.0;
    
    Eigen::VectorXd result = linear.forwardVector(input);
    QVERIFY(isVectorEqual(result, input));
    
    // Test derivative (should always be 1)
    QCOMPARE(linear.backward(0.0), 1.0);
    QCOMPARE(linear.backward(5.5), 1.0);
    QCOMPARE(linear.backward(-10.0), 1.0);
}

void TestActivationFunctions::testSoftmaxFunction()
{
    SoftmaxActivation softmax;
    
    // Softmax only works on vectors, test single values should throw or handle gracefully
    Eigen::VectorXd input1(3);
    input1 << 1.0, 2.0, 3.0;
    
    Eigen::VectorXd result1 = softmax.forwardVector(input1);
    
    // Check that probabilities sum to 1
    double sum = result1.sum();
    QVERIFY(isApproximatelyEqual(sum, 1.0, 1e-10));
    
    // Check that all probabilities are positive
    for (int i = 0; i < result1.size(); ++i) {
        QVERIFY(result1(i) > 0.0);
    }
    
    // Check that largest input gives largest probability
    int max_index;
    result1.maxCoeff(&max_index);
    QCOMPARE(max_index, 2);  // input[2] = 3.0 was the largest
    
    // Test numerical stability with large values
    Eigen::VectorXd large_input(3);
    large_input << 1000.0, 1001.0, 1002.0;
    
    Eigen::VectorXd result2 = softmax.forwardVector(large_input);
    QVERIFY(isApproximatelyEqual(result2.sum(), 1.0, 1e-10));
    QVERIFY(!result2.hasNaN());
    
    // Test with negative values
    Eigen::VectorXd negative_input(3);
    negative_input << -1.0, -2.0, -3.0;
    
    Eigen::VectorXd result3 = softmax.forwardVector(negative_input);
    QVERIFY(isApproximatelyEqual(result3.sum(), 1.0, 1e-10));
    
    // Largest (least negative) should have highest probability
    result3.maxCoeff(&max_index);
    QCOMPARE(max_index, 0);  // input[0] = -1.0 was the largest
}

void TestActivationFunctions::testActivationFactory()
{
    // Test factory function for all activation types
    auto relu = createActivationFunction("relu");
    QVERIFY(relu != nullptr);
    QCOMPARE(relu->forward(2.0), 2.0);
    QCOMPARE(relu->forward(-1.0), 0.0);
    
    auto sigmoid = createActivationFunction("sigmoid");
    QVERIFY(sigmoid != nullptr);
    QVERIFY(isApproximatelyEqual(sigmoid->forward(0.0), 0.5, 1e-6));
    
    auto tanh_func = createActivationFunction("tanh");
    QVERIFY(tanh_func != nullptr);
    QVERIFY(isApproximatelyEqual(tanh_func->forward(0.0), 0.0, 1e-6));
    
    auto linear = createActivationFunction("linear");
    QVERIFY(linear != nullptr);
    QCOMPARE(linear->forward(3.14), 3.14);
    
    auto softmax = createActivationFunction("softmax");
    QVERIFY(softmax != nullptr);
    
    // Test case insensitive
    auto relu_caps = createActivationFunction("RELU");
    QVERIFY(relu_caps != nullptr);
    
    auto relu_mixed = createActivationFunction("ReLU");
    QVERIFY(relu_mixed != nullptr);
    
    // Test invalid activation function
    auto invalid = createActivationFunction("invalid");
    QVERIFY(invalid != nullptr);  // Should return linear as default
}

void TestActivationFunctions::testEdgeCases()
{
    // Test with very small numbers
    SigmoidActivation sigmoid;
    QVERIFY(sigmoid.forward(-1000.0) > 0.0);  // Should not underflow to exactly 0
    QVERIFY(sigmoid.forward(1000.0) < 1.0);   // Should not overflow to exactly 1
    
    // Test ReLU with NaN (should handle gracefully)
    ReLUActivation relu;
    double nan_val = std::numeric_limits<double>::quiet_NaN();
    // Note: ReLU with NaN input behavior depends on implementation
    // We just verify it doesn't crash
    double relu_nan_result = relu.forward(nan_val);
    Q_UNUSED(relu_nan_result);  // May be NaN, which is acceptable
    
    // Test with infinity
    QCOMPARE(relu.forward(std::numeric_limits<double>::infinity()), 
             std::numeric_limits<double>::infinity());
    QCOMPARE(relu.forward(-std::numeric_limits<double>::infinity()), 0.0);
    
    // Test empty vector for softmax
    SoftmaxActivation softmax;
    Eigen::VectorXd empty_vec(0);
    Eigen::VectorXd empty_result = softmax.forwardVector(empty_vec);
    QCOMPARE(empty_result.size(), 0);
}

void TestActivationFunctions::testDerivatives()
{
    // Test ReLU derivative
    ReLUActivation relu;
    QCOMPARE(relu.backward(5.0), 1.0);   // x > 0
    QCOMPARE(relu.backward(-2.0), 0.0);  // x < 0
    QCOMPARE(relu.backward(0.0), 0.0);   // x = 0
    
    // Test sigmoid derivative numerically
    SigmoidActivation sigmoid;
    double h = 1e-7;
    double x = 1.5;
    double numerical_derivative = (sigmoid.forward(x + h) - sigmoid.forward(x - h)) / (2 * h);
    QVERIFY(isApproximatelyEqual(sigmoid.backward(x), numerical_derivative, 1e-6));
    
    // Test tanh derivative numerically
    TanhActivation tanh_func;
    x = 0.8;
    numerical_derivative = (tanh_func.forward(x + h) - tanh_func.forward(x - h)) / (2 * h);
    QVERIFY(isApproximatelyEqual(tanh_func.backward(x), numerical_derivative, 1e-6));
    
    // Test linear derivative
    LinearActivation linear;
    QCOMPARE(linear.backward(0.0), 1.0);
    QCOMPARE(linear.backward(100.0), 1.0);
    QCOMPARE(linear.backward(-50.0), 1.0);
}

bool TestActivationFunctions::isApproximatelyEqual(double a, double b, double tolerance)
{
    return std::abs(a - b) < tolerance;
}

bool TestActivationFunctions::isVectorEqual(const Eigen::VectorXd& v1, const Eigen::VectorXd& v2, double tolerance)
{
    if (v1.size() != v2.size()) return false;
    
    for (int i = 0; i < v1.size(); ++i) {
        if (!isApproximatelyEqual(v1(i), v2(i), tolerance)) {
            return false;
        }
    }
    return true;
}

QTEST_MAIN(TestActivationFunctions)
#include "test_activation_functions.moc"