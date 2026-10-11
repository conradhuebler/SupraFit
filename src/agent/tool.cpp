// tool.cpp - Value types of the agent tool layer.
// Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
//
// Claude Generated 2026 - see tool.h.

#include "tool.h"

bool needsApprovalAt(ToolEffect effect, ToolAutonomy autonomy)
{
    switch (autonomy) {
    case ToolAutonomy::Ask:
        return effect != ToolEffect::Read && effect != ToolEffect::Display;
    case ToolAutonomy::InProgram:
        return effect == ToolEffect::FileWrite || effect == ToolEffect::Process;
    case ToolAutonomy::Full:
        return false;
    }
    return true;
}

QString toolAutonomyName(ToolAutonomy autonomy)
{
    switch (autonomy) {
    case ToolAutonomy::Ask:       return QStringLiteral("ask");
    case ToolAutonomy::InProgram: return QStringLiteral("in-program");
    case ToolAutonomy::Full:      return QStringLiteral("full");
    }
    return QStringLiteral("unknown");
}

QString toolEffectName(ToolEffect effect)
{
    switch (effect) {
    case ToolEffect::Read:      return QStringLiteral("read");
    case ToolEffect::Display:   return QStringLiteral("display");
    case ToolEffect::Mutate:    return QStringLiteral("mutate");
    case ToolEffect::Compute:   return QStringLiteral("compute");
    case ToolEffect::FileWrite: return QStringLiteral("file-write");
    case ToolEffect::Process:   return QStringLiteral("process");
    }
    return QStringLiteral("unknown");
}

QString toolAffinityName(ToolAffinity affinity)
{
    switch (affinity) {
    case ToolAffinity::Any: return QStringLiteral("any");
    case ToolAffinity::Gui: return QStringLiteral("gui");
    }
    return QStringLiteral("unknown");
}
