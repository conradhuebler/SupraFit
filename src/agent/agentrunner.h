// agentrunner.h - Headless agent run: one question, one answer, no GUI.
// Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
//
// Claude Generated 2026 - Wires registry, dispatcher, client and session for the
// command line (suprafit_cli --agent). Projects must already be loaded into the
// ProjectManager. Blocks in a local event loop until the model has answered.
#pragma once

#include "tool.h"

#include <QString>

struct AgentRunOptions {
    QString question;
    QString profileName;  ///< empty = the config's active profile
    QString model;        ///< empty = the profile's model (suprafit_cli --model)
    QString configPath;   ///< empty = LlmConfig::defaultPath()
    ToolAutonomy autonomy = ToolAutonomy::InProgram;  ///< nothing in-process asks
    bool showReasoning = false;
    int maxIterations = 0;  ///< 0 = the profile's value
    bool storeInProjects = true;  ///< record the run as a chat in the projects (save with -o)
    QString appVersion, gitCommit;  ///< written into the chat record
};

/// System prompt for the SupraFit analysis agent. Claude Generated
QString agentSystemPrompt();

/// Run the agent. Returns 0 on an answer, 1 on a configuration/transport error.
/// Answer and tool trace go to stdout, errors to stderr.
int runAgent(const AgentRunOptions& options);

/// suprafit_cli --list-profiles: the profiles of the config, the active one marked. Claude Generated.
int listAgentProfiles(const QString& configPath);
/// suprafit_cli --list-models: ask the selected profile's endpoint for its models. Claude Generated.
int listAgentModels(const AgentRunOptions& options);
