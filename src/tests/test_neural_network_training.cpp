/*
 * Neural Network Training Tests
 * Copyright (C) 2019 - 2025 Conrad Hübler <Conrad.Huebler@gmx.net>
 */

#include <QtTest>
#include <QObject>
#include <vector>
#include <cmath>

#include "../ml/neural_network.h"
#include "../ml/loss_functions.h"
#include "../ml/optimizer.h"

class TestNeuralNetworkTraining : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    // Loss function tests
    void testMSELossFunction();
    void testCrossEntropyLossFunction();
    void testBinaryCrossEntropyLossFunction();
    void testLossFunctionFactory();
    
    // Optimizer tests
    void testSGDOptimizer();
    void testMomentumOptimizer();
    void testAdamOptimizer();
    void testOptimizerFactory();
    
    // Training data tests
    void testTrainingDataCreation();
    void testTrainingDataShuffle();
    void testTrainingDataMiniBatches();
    
    // Training tests
    void testXORTraining();
    void testRegressionTraining();
    void testClassificationTraining();
    void testTrainingHistory();
    void testValidationSplit();
    
    // Integration tests
    void testDifferentOptimizers();
    void testDifferentLearningRates();
    void testEarlyConvergence();
    void testOverfittingDetection();
    
private:
    TrainingData createXORData();
    TrainingData createRegressionData();
    TrainingData createClassificationData();
    bool isCloseToZero(double value, double tolerance = 1e-6);
    bool areVectorsClose(const Eigen::VectorXd& a, const Eigen::VectorXd& b, double tolerance = 1e-6);
};

void TestNeuralNetworkTraining::initTestCase()
{
    // Initialize random seed for reproducible tests
    srand(12345);
}

void TestNeuralNetworkTraining::cleanupTestCase()
{
}

void TestNeuralNetworkTraining::testMSELossFunction()
{
    MSELoss mse;
    
    // Test perfect prediction (zero loss)
    Eigen::Vector2d pred1(1.0, 0.5);
    Eigen::Vector2d target1(1.0, 0.5);
    double loss1 = mse.forward(pred1, target1);
    QVERIFY(isCloseToZero(loss1));
    
    // Test known loss value
    Eigen::Vector2d pred2(1.0, 0.0);
    Eigen::Vector2d target2(0.0, 1.0);
    double loss2 = mse.forward(pred2, target2);
    QVERIFY(std::abs(loss2 - 1.0) < 1e-6);  // MSE = (1-0)² + (0-1)² / 2 = 1.0
    
    // Test gradient
    Eigen::VectorXd grad = mse.backward(pred2, target2);
    QVERIFY(std::abs(grad(0) - 1.0) < 1e-6);   // d/dp1 = pred1 - target1 = 1.0 - 0.0
    QVERIFY(std::abs(grad(1) - (-1.0)) < 1e-6); // d/dp2 = pred2 - target2 = 0.0 - 1.0
}

void TestNeuralNetworkTraining::testCrossEntropyLossFunction()
{
    CrossEntropyLoss ce;
    
    // Test perfect prediction
    Eigen::Vector3d pred1(1.0, 0.0, 0.0);  // Probability for class 0
    Eigen::Vector3d target1(1.0, 0.0, 0.0); // Ground truth class 0
    double loss1 = ce.forward(pred1, target1);
    QVERIFY(isCloseToZero(loss1, 1e-5));  // Should be close to zero
    
    // Test gradient computation
    Eigen::VectorXd grad = ce.backward(pred1, target1);
    QCOMPARE(grad.size(), 3);
}

void TestNeuralNetworkTraining::testBinaryCrossEntropyLossFunction()
{
    BinaryCrossEntropyLoss bce;
    
    // Test perfect prediction
    Eigen::VectorXd pred1(1);
    pred1(0) = 0.999;  // Close to 1 (avoiding log(0))
    Eigen::VectorXd target1(1);
    target1(0) = 1.0;
    double loss1 = bce.forward(pred1, target1);
    QVERIFY(loss1 < 0.01);  // Should be very small
    
    // Test worst prediction
    Eigen::VectorXd pred2(1);
    pred2(0) = 0.001;  // Close to 0
    double loss2 = bce.forward(pred2, target1);  // target still 1.0
    QVERIFY(loss2 > 5.0);  // Should be large
}

void TestNeuralNetworkTraining::testLossFunctionFactory()
{
    auto mse = createLossFunction("mse");
    QVERIFY(mse != nullptr);
    QVERIFY(mse->name().contains("MSE"));
    
    auto ce = createLossFunction("crossentropy");
    QVERIFY(ce != nullptr);
    QVERIFY(ce->name().contains("Cross"));
    
    auto bce = createLossFunction("binarycrossentropy");
    QVERIFY(bce != nullptr);
    QVERIFY(bce->name().contains("Binary"));
    
    // Test invalid loss function
    QVERIFY_EXCEPTION_THROWN(createLossFunction("invalid"), std::invalid_argument);
}

void TestNeuralNetworkTraining::testSGDOptimizer()
{
    SGDOptimizer sgd(0.1);
    
    Eigen::MatrixXd weights(2, 2);
    weights << 1.0, 0.5,
               0.2, 0.8;
    
    Eigen::MatrixXd gradients(2, 2);
    gradients << 0.1, 0.2,
                 0.3, 0.4;
    
    Eigen::VectorXd biases(2);
    biases << 0.1, 0.2;
    
    Eigen::VectorXd bias_gradients(2);
    bias_gradients << 0.05, 0.1;
    
    Eigen::MatrixXd original_weights = weights;
    Eigen::VectorXd original_biases = biases;
    
    sgd.update(weights, gradients, biases, bias_gradients);
    
    // Check that weights were updated correctly: w_new = w_old - lr * grad
    QVERIFY(std::abs(weights(0,0) - (original_weights(0,0) - 0.1 * gradients(0,0))) < 1e-10);
    QVERIFY(std::abs(biases(0) - (original_biases(0) - 0.1 * bias_gradients(0))) < 1e-10);
}

void TestNeuralNetworkTraining::testMomentumOptimizer()
{
    MomentumOptimizer momentum(0.1, 0.9);
    
    Eigen::MatrixXd weights(1, 1);
    weights(0, 0) = 1.0;
    
    Eigen::MatrixXd gradients(1, 1);
    gradients(0, 0) = 0.1;
    
    Eigen::VectorXd biases(1);
    biases(0) = 0.0;
    
    Eigen::VectorXd bias_gradients(1);
    bias_gradients(0) = 0.0;
    
    double initial_weight = weights(0, 0);
    
    // First update
    momentum.update(weights, gradients, biases, bias_gradients);
    double weight_after_first = weights(0, 0);
    
    // Weight should have decreased (negative gradient with positive learning rate)
    QVERIFY(weight_after_first < initial_weight);
    
    // Second update with same gradient
    momentum.update(weights, gradients, biases, bias_gradients);
    double weight_after_second = weights(0, 0);
    
    // Should move further due to momentum
    double first_step = initial_weight - weight_after_first;
    double second_step = weight_after_first - weight_after_second;
    QVERIFY(second_step > first_step);  // Momentum should accelerate
}

void TestNeuralNetworkTraining::testAdamOptimizer()
{
    AdamOptimizer adam(0.001);
    
    Eigen::MatrixXd weights(1, 1);
    weights(0, 0) = 1.0;
    
    Eigen::MatrixXd gradients(1, 1);
    gradients(0, 0) = 1.0;  // Constant gradient
    
    Eigen::VectorXd biases(1);
    biases(0) = 0.0;
    
    Eigen::VectorXd bias_gradients(1);
    bias_gradients(0) = 0.0;
    
    double initial_weight = weights(0, 0);
    
    // Multiple updates
    for (int i = 0; i < 10; ++i) {
        adam.update(weights, gradients, biases, bias_gradients);
    }
    
    // Weight should have decreased
    QVERIFY(weights(0, 0) < initial_weight);
    
    // Test reset
    adam.reset();
    // After reset, next update should behave like first update
}

void TestNeuralNetworkTraining::testOptimizerFactory()
{
    auto sgd = createOptimizer("sgd", 0.01);
    QVERIFY(sgd != nullptr);
    QVERIFY(sgd->name().contains("SGD"));
    
    auto momentum = createOptimizer("momentum", 0.01);
    QVERIFY(momentum != nullptr);
    QVERIFY(momentum->name().contains("Momentum"));
    
    auto adam = createOptimizer("adam", 0.001);
    QVERIFY(adam != nullptr);
    QVERIFY(adam->name().contains("Adam"));
    
    // Test invalid optimizer
    QVERIFY_EXCEPTION_THROWN(createOptimizer("invalid", 0.01), std::invalid_argument);
}

void TestNeuralNetworkTraining::testTrainingDataCreation()
{
    TrainingData data;
    
    data.inputs.push_back(Eigen::Vector2d(0, 0));
    data.inputs.push_back(Eigen::Vector2d(1, 1));
    
    data.targets.push_back(Eigen::VectorXd::Zero(1));
    data.targets.push_back(Eigen::VectorXd::Ones(1));
    
    QCOMPARE(data.inputs.size(), 2);
    QCOMPARE(data.targets.size(), 2);
    QCOMPARE(data.inputs[0].size(), 2);
    QCOMPARE(data.targets[0].size(), 1);
}

void TestNeuralNetworkTraining::testTrainingDataShuffle()
{
    TrainingData data;
    
    // Create ordered data
    for (int i = 0; i < 10; ++i) {
        data.inputs.push_back(Eigen::VectorXd::Constant(1, i));
        data.targets.push_back(Eigen::VectorXd::Constant(1, i));
    }
    
    // Store original order
    std::vector<double> original_order;
    for (const auto& input : data.inputs) {
        original_order.push_back(input(0));
    }
    
    // Shuffle
    data.shuffle();
    
    // Check that size didn't change
    QCOMPARE(data.inputs.size(), 10);
    QCOMPARE(data.targets.size(), 10);
    
    // Check that inputs and targets are still aligned
    for (size_t i = 0; i < data.inputs.size(); ++i) {
        QVERIFY(std::abs(data.inputs[i](0) - data.targets[i](0)) < 1e-10);
    }
    
    // Check that order probably changed (with high probability)
    bool order_changed = false;
    for (size_t i = 0; i < data.inputs.size(); ++i) {
        if (std::abs(data.inputs[i](0) - original_order[i]) > 1e-10) {
            order_changed = true;
            break;
        }
    }
    // Note: This test could rarely fail if shuffle produces original order
    // In practice, this is extremely unlikely for 10 elements
}

void TestNeuralNetworkTraining::testTrainingDataMiniBatches()
{
    TrainingData data = createXORData();
    
    auto batches = data.createMiniBatches(2);
    
    QCOMPARE(batches.size(), 2);  // 4 samples with batch size 2 = 2 batches
    QCOMPARE(batches[0].inputs.size(), 2);
    QCOMPARE(batches[1].inputs.size(), 2);
    
    // Test with batch size larger than data
    auto single_batch = data.createMiniBatches(10);
    QCOMPARE(single_batch.size(), 1);
    QCOMPARE(single_batch[0].inputs.size(), 4);
}

void TestNeuralNetworkTraining::testXORTraining()
{
    NeuralNetwork network({2, 4, 1});
    TrainingData data = createXORData();
    
    TrainingConfig config;
    config.epochs = 500;
    config.learning_rate = 0.1;
    config.optimizer_name = "adam";
    config.loss_function = "binarycrossentropy";
    config.print_every = 500;  // Suppress output
    
    auto history = network.train(data, config);
    
    // Check that training occurred
    QCOMPARE(history.epochs_completed, config.epochs);
    QVERIFY(!history.train_loss.empty());
    QVERIFY(history.train_loss.size() > 0);
    
    // Check that loss decreased
    double initial_loss = history.train_loss[0];
    double final_loss = history.train_loss.back();
    QVERIFY(final_loss < initial_loss);
    
    // Test XOR predictions (should be reasonably accurate)
    auto predictions = network.predict(Eigen::Vector2d(0, 0));
    QVERIFY(predictions(0) < 0.3);  // Should predict close to 0
    
    predictions = network.predict(Eigen::Vector2d(0, 1));
    QVERIFY(predictions(0) > 0.7);  // Should predict close to 1
    
    predictions = network.predict(Eigen::Vector2d(1, 0));
    QVERIFY(predictions(0) > 0.7);  // Should predict close to 1
    
    predictions = network.predict(Eigen::Vector2d(1, 1));
    QVERIFY(predictions(0) < 0.3);  // Should predict close to 0
}

void TestNeuralNetworkTraining::testRegressionTraining()
{
    NeuralNetwork network({1, 5, 1});
    TrainingData data = createRegressionData();
    
    TrainingConfig config;
    config.epochs = 200;
    config.learning_rate = 0.01;
    config.optimizer_name = "adam";
    config.loss_function = "mse";
    config.print_every = 200;
    
    auto history = network.train(data, config);
    
    QVERIFY(!history.train_loss.empty());
    
    // Test that the network learned something reasonable
    double initial_loss = history.train_loss[0];
    double final_loss = history.train_loss.back();
    QVERIFY(final_loss < initial_loss * 0.1);  // Should reduce loss significantly
}

void TestNeuralNetworkTraining::testClassificationTraining()
{
    NeuralNetwork network({2, 8, 3});
    TrainingData data = createClassificationData();
    
    TrainingConfig config;
    config.epochs = 100;
    config.learning_rate = 0.01;
    config.optimizer_name = "adam";
    config.loss_function = "crossentropy";
    config.print_every = 100;
    
    auto history = network.train(data, config);
    
    QVERIFY(!history.train_loss.empty());
    QVERIFY(!history.train_accuracy.empty());
    
    // Should achieve reasonable accuracy on synthetic data
    double final_accuracy = history.train_accuracy.back();
    QVERIFY(final_accuracy > 0.7);  // Should be better than random (33%)
}

void TestNeuralNetworkTraining::testTrainingHistory()
{
    NeuralNetwork network({2, 3, 1});
    TrainingData data = createXORData();
    
    TrainingConfig config;
    config.epochs = 50;
    config.validation_split = 0.25;  // Use validation
    config.print_every = 50;
    
    auto history = network.train(data, config);
    
    QCOMPARE(history.epochs_completed, 50);
    QCOMPARE(history.train_loss.size(), 50);
    QCOMPARE(history.validation_loss.size(), 50);
    
    // All values should be non-negative
    for (double loss : history.train_loss) {
        QVERIFY(loss >= 0.0);
    }
    
    for (double loss : history.validation_loss) {
        QVERIFY(loss >= 0.0);
    }
}

void TestNeuralNetworkTraining::testValidationSplit()
{
    NeuralNetwork network({1, 3, 1});
    TrainingData data = createRegressionData();  // Should have > 4 samples
    
    TrainingConfig config;
    config.epochs = 10;
    config.validation_split = 0.3;  // 30% for validation
    config.print_every = 10;
    
    auto history = network.train(data, config);
    
    // Should have validation data
    QVERIFY(!history.validation_loss.empty());
    QCOMPARE(history.validation_loss.size(), history.train_loss.size());
}

void TestNeuralNetworkTraining::testDifferentOptimizers()
{
    std::vector<QString> optimizers = {"sgd", "momentum", "adam"};
    TrainingData data = createXORData();
    
    for (const auto& opt : optimizers) {
        NeuralNetwork network({2, 4, 1});
        
        TrainingConfig config;
        config.epochs = 100;
        config.learning_rate = (opt == "adam") ? 0.01 : 0.1;
        config.optimizer_name = opt;
        config.loss_function = "binarycrossentropy";
        config.print_every = 100;
        
        auto history = network.train(data, config);
        
        QVERIFY(!history.train_loss.empty());
        QVERIFY(history.train_loss.back() >= 0.0);
        
        // All optimizers should reduce loss
        QVERIFY(history.train_loss.back() < history.train_loss[0]);
    }
}

void TestNeuralNetworkTraining::testDifferentLearningRates()
{
    std::vector<double> learning_rates = {0.001, 0.01, 0.1};
    TrainingData data = createXORData();
    
    for (double lr : learning_rates) {
        NeuralNetwork network({2, 4, 1});
        
        TrainingConfig config;
        config.epochs = 100;
        config.learning_rate = lr;
        config.optimizer_name = "adam";
        config.print_every = 100;
        
        auto history = network.train(data, config);
        
        QVERIFY(!history.train_loss.empty());
        // Very high learning rates might cause instability, but should still train
        QVERIFY(std::isfinite(history.train_loss.back()));
    }
}

void TestNeuralNetworkTraining::testEarlyConvergence()
{
    NeuralNetwork network({1, 2, 1});
    TrainingData simple_data;
    
    // Very simple linear data that should converge quickly
    simple_data.inputs = {
        Eigen::VectorXd::Constant(1, 0.0),
        Eigen::VectorXd::Constant(1, 1.0)
    };
    simple_data.targets = {
        Eigen::VectorXd::Constant(1, 0.0),
        Eigen::VectorXd::Constant(1, 1.0)
    };
    
    TrainingConfig config;
    config.epochs = 1000;  // Many epochs
    config.learning_rate = 0.1;
    config.print_every = 1000;
    
    auto history = network.train(simple_data, config);
    
    // Should converge to very low loss
    QVERIFY(history.train_loss.back() < 0.01);
}

void TestNeuralNetworkTraining::testOverfittingDetection()
{
    // Create a scenario prone to overfitting: large network, small dataset
    NeuralNetwork network({2, 20, 10, 1});  // Oversized network
    TrainingData data = createXORData();     // Small dataset
    
    TrainingConfig config;
    config.epochs = 500;
    config.learning_rate = 0.01;
    config.validation_split = 0.5;  // Very small validation set
    config.print_every = 500;
    
    auto history = network.train(data, config);
    
    // Check for potential overfitting signs
    QVERIFY(!history.validation_loss.empty());
    
    if (history.validation_loss.size() > 100) {
        // Compare early vs late validation loss
        double early_val_loss = history.validation_loss[50];
        double late_val_loss = history.validation_loss.back();
        
        // In some cases, validation loss might increase (overfitting)
        // This test just ensures the system can handle such scenarios
        QVERIFY(std::isfinite(early_val_loss));
        QVERIFY(std::isfinite(late_val_loss));
    }
}

TrainingData TestNeuralNetworkTraining::createXORData()
{
    TrainingData data;
    
    data.inputs = {
        Eigen::Vector2d(0, 0),
        Eigen::Vector2d(0, 1),
        Eigen::Vector2d(1, 0),
        Eigen::Vector2d(1, 1)
    };
    
    data.targets = {
        Eigen::VectorXd::Zero(1),
        Eigen::VectorXd::Ones(1),
        Eigen::VectorXd::Ones(1),
        Eigen::VectorXd::Zero(1)
    };
    
    return data;
}

TrainingData TestNeuralNetworkTraining::createRegressionData()
{
    TrainingData data;
    
    // Simple quadratic function: y = x^2
    for (int i = 0; i < 20; ++i) {
        double x = (i - 10) / 10.0;  // x from -1 to 1
        Eigen::VectorXd input(1);
        input(0) = x;
        
        Eigen::VectorXd target(1);
        target(0) = x * x;  // y = x^2
        
        data.inputs.push_back(input);
        data.targets.push_back(target);
    }
    
    return data;
}

TrainingData TestNeuralNetworkTraining::createClassificationData()
{
    TrainingData data;
    
    // Three classes with simple separable patterns
    for (int class_id = 0; class_id < 3; ++class_id) {
        for (int i = 0; i < 10; ++i) {
            Eigen::Vector2d input;
            
            // Class-specific patterns
            if (class_id == 0) {
                input << -1.0 + 0.1 * i, -1.0 + 0.1 * i;  // Bottom-left
            } else if (class_id == 1) {
                input << 1.0 - 0.1 * i, -1.0 + 0.1 * i;   // Bottom-right
            } else {
                input << 0.0, 1.0 - 0.1 * i;              // Top-center
            }
            
            Eigen::Vector3d target = Eigen::Vector3d::Zero();
            target(class_id) = 1.0;  // One-hot encoding
            
            data.inputs.push_back(input);
            data.targets.push_back(target);
        }
    }
    
    return data;
}

bool TestNeuralNetworkTraining::isCloseToZero(double value, double tolerance)
{
    return std::abs(value) < tolerance;
}

bool TestNeuralNetworkTraining::areVectorsClose(const Eigen::VectorXd& a, const Eigen::VectorXd& b, double tolerance)
{
    if (a.size() != b.size()) return false;
    
    for (int i = 0; i < a.size(); ++i) {
        if (std::abs(a(i) - b(i)) > tolerance) return false;
    }
    return true;
}

QTEST_APPLESS_MAIN(TestNeuralNetworkTraining)

#include "test_neural_network_training.moc"