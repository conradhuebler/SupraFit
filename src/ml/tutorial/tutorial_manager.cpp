/*
 * Tutorial Manager for Neural Network Education
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

#include "tutorial_manager.h"
#include <iostream>
#include <fmt/format.h>

TutorialManager::TutorialManager(TutorialLevel level, bool interactive)
    : level_(level)
    , interactive_(interactive)
    , step_counter_(0)
{
}

void TutorialManager::explainConcept(const QString& title, const QString& explanation)
{
    fmt::print("\n📚 {}\n", title.toStdString());
    fmt::print("{}\n", explanation.toStdString());
    
    if (interactive_) {
        waitForUser();
    }
}

void TutorialManager::showStep(const QString& stepDescription)
{
    step_counter_++;
    fmt::print("\n🔢 Step {}: {}\n", step_counter_, stepDescription.toStdString());
    
    if (interactive_) {
        waitForUser();
    }
}

void TutorialManager::showMatrix(const QString& name, const Eigen::MatrixXd& matrix)
{
    fmt::print("\n🧮 Matrix {}:\n", name.toStdString());
    fmt::print("{}\n", formatMatrix(matrix).toStdString());
    
    if (level_ != TutorialLevel::Basic) {
        fmt::print("   Dimensions: {} × {}\n", matrix.rows(), matrix.cols());
    }
    
    if (interactive_) {
        waitForUser();
    }
}

void TutorialManager::showVector(const QString& name, const Eigen::VectorXd& vector)
{
    fmt::print("\n📊 Vector {}:\n", name.toStdString());
    fmt::print("{}\n", formatVector(vector).toStdString());
    
    if (level_ != TutorialLevel::Basic) {
        fmt::print("   Size: {}\n", vector.size());
    }
    
    if (interactive_) {
        waitForUser();
    }
}

void TutorialManager::waitForUser(const QString& prompt)
{
    fmt::print("{}", prompt.toStdString());
    std::cin.get();
}

void TutorialManager::askQuestion(const QString& question, const QString& answer)
{
    fmt::print("\n❓ {}\n", question.toStdString());
    
    if (interactive_) {
        fmt::print("Think about it, then press Enter for the answer...");
        std::cin.get();
    }
    
    fmt::print("💡 Answer: {}\n", answer.toStdString());
    
    if (interactive_) {
        waitForUser();
    }
}

QString TutorialManager::formatMatrix(const Eigen::MatrixXd& matrix, int precision) const
{
    QString result;
    
    for (int i = 0; i < matrix.rows(); ++i) {
        result += "   [";
        for (int j = 0; j < matrix.cols(); ++j) {
            result += QString(" %1").arg(matrix(i, j), 8, 'f', precision);
        }
        result += " ]\n";
    }
    
    return result;
}

QString TutorialManager::formatVector(const Eigen::VectorXd& vector, int precision) const
{
    QString result = "   [";
    
    for (int i = 0; i < vector.size(); ++i) {
        result += QString(" %1").arg(vector(i), 8, 'f', precision);
    }
    
    result += " ]\n";
    return result;
}