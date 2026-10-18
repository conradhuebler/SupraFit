// agenthelp.cpp - see agenthelp.h
// Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
//
// Claude Generated 2026
#include "agenthelp.h"

#include <QFile>

QString agentHelpText()
{
    Q_INIT_RESOURCE(agenthelp); // a static library's resource is not linked in by itself
    QFile file(QStringLiteral(":/agent/ASSISTANT_HELP.md"));
    return file.open(QIODevice::ReadOnly) ? QString::fromUtf8(file.readAll()) : QString();
}
