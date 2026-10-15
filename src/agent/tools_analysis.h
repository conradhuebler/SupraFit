// tools_analysis.h - Compute tools: fit, compare, post-process, BC50.
// Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
//
// Claude Generated 2026 - see tools_analysis.cpp for the contract.
#pragma once

#include <QPointer>

namespace SupraFit { class ProjectManager; }
class ToolRegistry;

/// Register the Phase-5 compute tools into @p registry. @p pm is borrowed and must
/// outlive the registry. Returns the number of tools registered.
///
/// Tools: fit_model, fit_models, compare_models_aic, run_postfit_analysis,
///        calculate_bc50.
int registerAnalysisTools(ToolRegistry& registry, QPointer<SupraFit::ProjectManager> pm);
