/*
 * Neural Network Activation Functions for SupraFit
 * Copyright (C) 2025 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "activation_functions.h"

std::unique_ptr<ActivationFunction> createActivationFunction(const QString& name)
{
    QString lowerName = name.toLower();
    
    if (lowerName == "relu") {
        return std::make_unique<ReLUActivation>();
    } else if (lowerName == "sigmoid") {
        return std::make_unique<SigmoidActivation>();
    } else if (lowerName == "tanh") {
        return std::make_unique<TanhActivation>();
    } else if (lowerName == "softmax") {
        return std::make_unique<SoftmaxActivation>();
    } else if (lowerName == "linear") {
        return std::make_unique<LinearActivation>();
    }
    
    // Default to ReLU for unknown activation functions
    return std::make_unique<ReLUActivation>();
}