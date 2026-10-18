// agenthelp.h - SupraFit's how-to help for the assistant.
// Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
//
// Claude Generated 2026 - docs/ASSISTANT_HELP.md is compiled into the binary. How it reaches the model
// (tool get_help, or the whole text in the system prompt) is CuteChat's business (ChatHelp, the chat
// dock's menu); this only supplies the text. Every user-visible feature is described in that file:
// see CLAUDE.md, "Where Things Go".
#pragma once

#include <QString>

/// The text of docs/ASSISTANT_HELP.md; empty if the resource is missing.
QString agentHelpText();
