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

#pragma once

#include <QString>
#include <Eigen/Dense>

/**
 * @brief Tutorial difficulty levels
 */
enum class TutorialLevel {
    Basic,      // Simple explanations, minimal math
    Intermediate, // Moderate detail, some equations
    Advanced    // Full mathematical detail
};

/**
 * @brief Tutorial Manager for Interactive Learning
 * 
 * Provides educational explanations and step-by-step guidance
 * for neural network concepts. Designed to make machine learning
 * accessible to chemistry and physics researchers.
 * 
 * Features:
 * - Interactive step-by-step execution
 * - Matrix operation visualization  
 * - Scientific interpretation of results
 * - Customizable detail level
 * 
 * Claude Generated - 2025
 */
class TutorialManager {
public:
    /**
     * @brief Constructor
     * @param level Tutorial detail level
     * @param interactive Whether to pause for user input
     */
    TutorialManager(TutorialLevel level = TutorialLevel::Basic, 
                   bool interactive = false);

    /**
     * @brief Set tutorial parameters
     */
    void setLevel(TutorialLevel level) { level_ = level; }
    void setInteractive(bool interactive) { interactive_ = interactive; }

    /**
     * @brief Educational explanations
     */
    void explainConcept(const QString& title, const QString& explanation);
    void showStep(const QString& stepDescription);
    void showMatrix(const QString& name, const Eigen::MatrixXd& matrix);
    void showVector(const QString& name, const Eigen::VectorXd& vector);

    /**
     * @brief Interactive features
     */
    void waitForUser(const QString& prompt = "Press Enter to continue...");
    void askQuestion(const QString& question, const QString& answer);

    /**
     * @brief Accessors (Claude Generated)
     */
    TutorialLevel getLevel() const { return level_; }
    bool isInteractive() const { return interactive_; }

public:
    TutorialLevel level_;  // Public for neural network access

private:
    bool interactive_;
    int step_counter_;

    /**
     * @brief Formatting helpers
     */
    QString formatMatrix(const Eigen::MatrixXd& matrix, int precision = 4) const;
    QString formatVector(const Eigen::VectorXd& vector, int precision = 4) const;
};