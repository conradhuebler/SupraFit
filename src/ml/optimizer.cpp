/*
 * Optimizer Implementation
 * Copyright (C) 2025 Conrad Hübler <Conrad.Huebler@gmx.net>
 */

#include "optimizer.h"

std::unique_ptr<Optimizer> createOptimizer(const QString& name, double learning_rate)
{
    QString lower_name = name.toLower();
    
    if (lower_name == "sgd") {
        return std::make_unique<SGDOptimizer>(learning_rate);
    } else if (lower_name == "momentum") {
        return std::make_unique<MomentumOptimizer>(learning_rate);
    } else if (lower_name == "adam") {
        return std::make_unique<AdamOptimizer>(learning_rate);
    } else {
        throw std::invalid_argument("Unknown optimizer: " + name.toStdString());
    }
}