/*
 * SupraFit - the assistant's program help stays complete (no net, no GUI)
 * Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
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

/* docs/ASSISTANT_HELP.md is what the assistant answers "how do I ...?" from. The project rule is that
 * every user-visible feature is described there in the same change (CLAUDE.md, "Where Things Go").
 * The part of that rule a test can check is checked here: the compiled-in document parses into
 * numbered sections, and every tool the assistant has is named in it. A new tool without a line in
 * the help fails this test.
 * Claude Generated (2026). */

#include <QtCore/QSet>
#include <QtTest/QtTest>

#include <cutechat/chathelp.h>
#include <cutechat/toolregistry.h>
#include "src/agent/agenthelp.h"
#include "src/agent/tools_analysis.h"
#include "src/agent/tools_inspect.h"
#include "src/agent/tools_project.h"
#include "src/core/projectmanager.h"

class AgentHelpTest : public QObject {
    Q_OBJECT
private slots:
    void theHelpIsCompiledInAndCutIntoNumberedSections()
    {
        const ChatHelp help(agentHelpText());
        QVERIFY2(!help.isEmpty(), "docs/ASSISTANT_HELP.md is not in the binary (agenthelp.qrc)");
        QVERIFY2(help.sections().size() >= 8, "the help lost its sections");
        for (int i = 0; i < help.sections().size(); ++i) {
            QCOMPARE(help.sections().at(i).number, i + 1); // 1, 2, 3, ... without gaps
            QVERIFY2(help.sections().at(i).text.size() > 80, qPrintable(QStringLiteral("section %1 is empty").arg(i + 1)));
        }
        QVERIFY2(!help.text().contains(QLatin1String("Copyright")), "the provenance comment must be dropped");
        // small enough to be put into a prompt: this is the budget the setting's tooltip quotes
        QVERIFY2(help.estimatedTokens() < 8000, qPrintable(QStringLiteral("help grew to ~%1 tokens; shorten it").arg(help.estimatedTokens())));
    }

    void everyToolOfTheAssistantIsNamedInTheHelp()
    {
        const ChatHelp help(agentHelpText());
        ToolRegistry registry;
        auto& pm = SupraFit::ProjectManager::instance();
        registerProjectTools(registry, &pm);
        registerAnalysisTools(registry, &pm);
        registerInspectTools(registry, &pm);
        help.registerTool(registry);
        QVERIFY(registry.contains(QStringLiteral("get_help")));
        QVERIFY(registry.names().size() >= 20);
        QStringList missing;
        for (const QString& name : registry.names())
            if (!help.text().contains(QLatin1Char('`') + name + QLatin1Char('`')))
                missing << name;
        QVERIFY2(missing.isEmpty(),
            qPrintable(QStringLiteral("tools not described in docs/ASSISTANT_HELP.md (section 8): %1").arg(missing.join(QLatin1String(", ")))));
    }
};

QTEST_MAIN(AgentHelpTest)
#include "test_agent_help.moc"
