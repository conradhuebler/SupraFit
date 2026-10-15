// agentrunner.cpp - see agentrunner.h
// Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
//
// Claude Generated 2026
#include "agentrunner.h"

#include "llmclient.h"
#include "llmconfig.h"
#include "llmsession.h"
#include "toolregistry.h"
#include "tooldispatcher.h"
#include "tools_analysis.h"
#include "tools_project.h"

#include "src/core/projectmanager.h"

#include <QCoreApplication>
#include <QEventLoop>
#include <QJsonDocument>

#include <fmt/core.h>

#include <cstdio>

QString agentSystemPrompt()
{
    return QStringLiteral(
        "You are the analysis assistant of SupraFit, a program that fits binding "
        "models (supramolecular titrations, NMR/UV/ITC) and gives statistics on the fits. "
        "You work ONLY through the tools offered to you; never invent project ids, "
        "model indices or numbers. Start with list_projects when you do not know what "
        "is loaded. Prefer the stored fits (get_model_fit, compare_models_aic) before "
        "fitting again. When a project has no models yet, create them with add_model "
        "(ids from describe_model_catalog) and then fit them. When no catalogue model fits "
        "the chemistry, write your own with define_script_model (equation, parameter names) and fit it. "
        "To test mixed stoichiometries or self-aggregation, describe the species as reactions with "
        "add_reaction_model, fit every candidate and let the AIC decide. Compare models by AIC, mention SSE and whether the fit converged, "
        "and say plainly when a result is missing or a tool refused. "
        "Answer concisely in the language of the question.");
}

int runAgent(const AgentRunOptions& options)
{
    const QString path = options.configPath.isEmpty() ? LlmConfig::defaultPath() : options.configPath;
    LlmConfig config;
    QString error;
    if (!config.load(path, &error)) {
        // First use: leave a template to edit, and say where it is.
        QString werr;
        if (LlmConfig::writeExampleIfMissing(path, &werr))
            fmt::print(stderr, "No LLM config found; wrote an example to {}. Edit it and retry.\n", path.toStdString());
        else
            fmt::print(stderr, "Cannot read LLM config {}: {}\n", path.toStdString(), error.toStdString());
        return 1;
    }

    LlmProfile profile;
    const bool found = options.profileName.isEmpty() ? config.activeProfile(profile)
                                                     : config.profile(options.profileName, profile);
    if (!found) {
        fmt::print(stderr, "LLM profile '{}' not found in {}\n",
            options.profileName.isEmpty() ? "(active)" : options.profileName.toStdString(), path.toStdString());
        return 1;
    }

    ToolRegistry registry;
    registerProjectTools(registry, &SupraFit::ProjectManager::instance());
    registerAnalysisTools(registry, &SupraFit::ProjectManager::instance());
    ToolDispatcher dispatcher(&registry);
    fmt::print(stderr, "[agent] profile {} model {} | tool catalogue {} bytes\n",
        profile.name.toStdString(), profile.model.toStdString(), registry.catalogueBytes());

    LlmClient client;
    client.setProfile(profile);
    client.setApiKey(LlmConfig::apiKeyFor(profile));

    LlmSession session(&client, &registry, &dispatcher);
    session.setAutonomy(options.autonomy);
    session.setSystemPrompt(agentSystemPrompt());
    session.setMaxIterations(options.maxIterations > 0 ? options.maxIterations : profile.maxToolIterations);
    // Below the chosen autonomy a tool asks; a headless run has nobody to ask.
    session.setApprovalPolicy([](const ToolSpec& spec, const QJsonObject&) {
        fmt::print(stderr, "[refused] {} needs approval; raise the autonomy to allow it\n", spec.name.toStdString());
        return false;
    });

    int exitCode = 0;
    bool streamed = false;
    QEventLoop loop;
    QObject::connect(&session, &LlmSession::assistantChunk, &loop, [&](const QString& t) {
        streamed = true;
        fmt::print("{}", t.toStdString());
        std::fflush(stdout);
    });
    if (options.showReasoning)
        QObject::connect(&session, &LlmSession::reasoningChunk, &loop, [](const QString& t) {
            fmt::print(stderr, "{}", t.toStdString());
        });
    QObject::connect(&session, &LlmSession::toolStarted, &loop, [](const QString& n, const QJsonObject& a) {
        fmt::print(stderr, "\n[tool] {} {}\n", n.toStdString(),
            QJsonDocument(a).toJson(QJsonDocument::Compact).toStdString());
    });
    QObject::connect(&session, &LlmSession::toolFinished, &loop, [](const QString& n, const ToolResult& r) {
        fmt::print(stderr, "[tool] {} -> {}\n", n.toStdString(),
            r.ok ? "ok" : ("error: " + r.error).toStdString());
    });
    QObject::connect(&session, &LlmSession::assistantMessage, &loop, [&](const QString& text) {
        if (!streamed)  // endpoint without streaming
            fmt::print("{}", text.toStdString());
        streamed = false;
        fmt::print("\n");
    });
    QObject::connect(&session, &LlmSession::failed, &loop, [&](const QString& e) {
        fmt::print(stderr, "\nAgent failed: {}\n", e.toStdString());
        exitCode = 1;
        loop.quit();
    });
    QObject::connect(&session, &LlmSession::finished, &loop, &QEventLoop::quit);

    session.ask(options.question);
    if (session.isBusy())
        loop.exec();
    return exitCode;
}
