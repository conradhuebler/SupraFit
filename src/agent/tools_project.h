// tools_project.h - Read tools over ProjectManager.
// Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
//
// Claude Generated 2026 - see tools_project.cpp for the contract.
#pragma once

#include <QPointer>

namespace SupraFit { class ProjectManager; }
class ToolRegistry;

/// Register the Phase-4 read tools into @p registry. @p pm is borrowed and must
/// outlive the registry. Returns the number of tools registered.
///
/// Tools: list_projects, get_project_info, list_models, get_model_fit,
///        describe_model_catalog.
int registerProjectTools(ToolRegistry& registry, QPointer<SupraFit::ProjectManager> pm);
