/*
 * Loss Functions Implementation
 * Copyright (C) 2019 - 2025 Conrad Hübler <Conrad.Huebler@gmx.net>
 */

#include "loss_functions.h"
#include <algorithm>
#include <random>

std::unique_ptr<LossFunction> createLossFunction(const QString& name)
{
    QString lower_name = name.toLower();
    
    if (lower_name == "mse" || lower_name == "meansquarederror") {
        return std::make_unique<MeanSquaredErrorLoss>();
    } else if (lower_name == "crossentropy" || lower_name == "ce") {
        return std::make_unique<CrossEntropyLoss>();
    } else if (lower_name == "binarycrossentropy" || lower_name == "bce") {
        return std::make_unique<BinaryCrossEntropyLoss>();
    } else {
        throw std::invalid_argument("Unknown loss function: " + name.toStdString());
    }
}

std::vector<std::pair<std::vector<Eigen::VectorXd>, std::vector<Eigen::VectorXd>>> 
TrainingData::createMiniBatches(size_t batch_size) const
{
    std::vector<std::pair<std::vector<Eigen::VectorXd>, std::vector<Eigen::VectorXd>>> batches;
    
    if (inputs.size() != targets.size() || inputs.empty()) {
        return batches;
    }
    
    // Create indices and shuffle them
    std::vector<size_t> indices(inputs.size());
    std::iota(indices.begin(), indices.end(), 0);
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(indices.begin(), indices.end(), g);
    
    // Create batches
    for (size_t start = 0; start < indices.size(); start += batch_size) {
        size_t end = std::min(start + batch_size, indices.size());
        
        std::vector<Eigen::VectorXd> batch_inputs;
        std::vector<Eigen::VectorXd> batch_targets;
        
        for (size_t i = start; i < end; ++i) {
            batch_inputs.push_back(inputs[indices[i]]);
            batch_targets.push_back(targets[indices[i]]);
        }
        
        batches.emplace_back(std::move(batch_inputs), std::move(batch_targets));
    }
    
    return batches;
}