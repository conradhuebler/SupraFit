// tools_inspect.h - Tools that let the agent look INTO a project: fitted parameters, stored
// statistics, the data tables and the residuals, plus one tool to set a parameter value.
// Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
//
// Claude Generated 2026 - closes the gap found in the first live chat runs: the agent could rank
// models but could not name a fitted constant (no tool exposed the parameter values).
#pragma once

#include <QPointer>

#include <QJsonArray>
#include <QJsonObject>

class ToolRegistry;
namespace SupraFit {
class ProjectManager;
}

/// Registers get_model_parameters, get_model_statistics, get_data_table, get_residuals (all Read)
/// and set_model_parameter (Mutate). Returns the number registered (5).
int registerInspectTools(ToolRegistry& registry, QPointer<SupraFit::ProjectManager> pm);

/// Per-parameter summary (value, confidence interval, median, standard deviation) of one stored
/// statistic run; @p samples receives the largest sample count found.
QJsonArray summariseStatisticParameters(const QJsonObject& body, int* samples = nullptr);
