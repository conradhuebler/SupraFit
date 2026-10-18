// agentrunner.cpp - see agentrunner.h
// Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
//
// Claude Generated 2026
#include "agentrunner.h"
#include "agenthelp.h"

#include <cutechat/chathelp.h>
#include <cutechat/chatrecorder.h>
#include "projectchats.h"

#include <cutechat/llmclient.h>
#include <cutechat/llmconfig.h>
#include <cutechat/llmsession.h>
#include <cutechat/toolregistry.h>
#include <cutechat/tooldispatcher.h>
#include "tools_analysis.h"
#include "tools_inspect.h"
#include "tools_project.h"

#include "src/core/projectmanager.h"

#include <QCoreApplication>
#include <QEventLoop>
#include <QJsonDocument>
#include <QUrl>

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
        "and say plainly when a result is missing or a tool refused. Report parameter values "
        "(get_model_parameters), confidence intervals from stored statistics (get_model_statistics) and judge "
        "the quality of a fit with get_residuals; look at the data with get_data_table. "
        "Answer concisely in the language of the question.");
}

namespace {
bool loadProfile(const AgentRunOptions& options, LlmProfile& profile, QString& path)
{
    path = options.configPath.isEmpty() ? LlmConfig::defaultPath() : options.configPath;
    LlmConfig config;
    QString error;
    if (!config.load(path, &error)) {
        fmt::print(stderr, "Cannot read LLM config {}: {}\n", path.toStdString(), error.toStdString());
        return false;
    }
    const bool found = options.profileName.isEmpty() ? config.activeProfile(profile) : config.profile(options.profileName, profile);
    if (!found)
        fmt::print(stderr, "LLM profile '{}' not found in {}\n",
            options.profileName.isEmpty() ? "(active)" : options.profileName.toStdString(), path.toStdString());
    return found;
}
} // namespace

int listAgentProfiles(const QString& configPath)
{
    const QString path = configPath.isEmpty() ? LlmConfig::defaultPath() : configPath;
    LlmConfig config;
    QString error;
    if (!config.load(path, &error)) {
        fmt::print(stderr, "Cannot read LLM config {}: {}\n", path.toStdString(), error.toStdString());
        return 1;
    }
    fmt::print("LLM profiles in {}:\n", path.toStdString());
    for (const LlmProfile& p : config.profiles()) {
        const bool active = p.name == config.activeProfileName();
        fmt::print("{} {:<20} {:<45} model: {}{}\n", active ? "*" : " ", p.name.toStdString(), p.baseUrl.toStdString(),
            p.model.isEmpty() ? "(endpoint lists)" : p.model.toStdString(),
            p.apiKeyEnv.isEmpty() ? "" : (" | key from " + p.apiKeyEnv.toStdString() + (LlmConfig::apiKeyFor(p).isEmpty() ? " (not set)" : " (set)")));
    }
    return 0;
}

int listAgentModels(const AgentRunOptions& options)
{
    LlmProfile profile;
    QString path;
    if (!loadProfile(options, profile, path))
        return 1;
    const QString keyProblem = LlmConfig::missingKeyMessage(profile);
    if (!keyProblem.isEmpty()) {
        fmt::print(stderr, "{}\n", keyProblem.toStdString());
        return 1;
    }
    LlmClient client;
    client.setProfile(profile);
    client.setApiKey(LlmConfig::apiKeyFor(profile));
    QStringList models;
    QString failure;
    QEventLoop loop;
    QObject::connect(&client, &LlmClient::modelListFailed, &loop, [&failure](const QString& e) { failure = e; });
    QObject::connect(&client, &LlmClient::modelsListed, &loop, [&](const QStringList& m) {
        models = m;
        loop.quit();
    });
    client.listModels();
    loop.exec();
    if (!failure.isEmpty()) {
        fmt::print(stderr, "{}\n", failure.toStdString());
        return 1;
    }
    fmt::print("{} models at {} (profile {}):\n", models.size(), profile.baseUrl.toStdString(), profile.name.toStdString());
    for (const QString& m : models)
        fmt::print("  {}{}\n", m.toStdString(), m == profile.model ? "  (profile default)" : "");
    return 0;
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

    const QString keyProblem = LlmConfig::missingKeyMessage(profile);
    if (!keyProblem.isEmpty()) {
        fmt::print(stderr, "{}\n", keyProblem.toStdString());
        return 1;
    }
    if (!LlmConfig::isLocalEndpoint(profile))
        fmt::print(stderr, "[agent] remote endpoint {}: the conversation and the project data the agent reads are sent there\n",
            profile.baseUrl.toStdString());

    ToolRegistry registry;
    registerProjectTools(registry, &SupraFit::ProjectManager::instance());
    registerAnalysisTools(registry, &SupraFit::ProjectManager::instance());
    registerInspectTools(registry, &SupraFit::ProjectManager::instance());
    const ChatHelp help(agentHelpText());
    help.registerTool(registry); // get_help: how-to questions are answered from the program help
    ToolDispatcher dispatcher(&registry);
    LlmClient client;
    client.setProfile(profile);
    client.setApiKey(LlmConfig::apiKeyFor(profile));
    if (!options.model.isEmpty())
        client.setModel(options.model);
    fmt::print(stderr, "[agent] profile {} model {} | tool catalogue {} bytes\n",
        profile.name.toStdString(), client.model().toStdString(), registry.catalogueBytes());

    LlmSession session(&client, &registry, &dispatcher);
    session.setAutonomy(options.autonomy);
    session.setSystemPrompt(help.extendPrompt(agentSystemPrompt(), options.helpInPrompt ? ChatHelp::Mode::Prompt : ChatHelp::Mode::Tool));
    session.setMaxIterations(options.maxIterations > 0 ? options.maxIterations : profile.maxToolIterations);
    // Audit trail (WP11): the run is recorded like a chat in the dock and stored in the
    // projects it belongs to, so a saved project shows what the agent did.
    ChatLog chat;
    chat.setProfile(profile.name);
    chat.setModel(client.model());
    chat.setEndpointHost(QUrl(profile.baseUrl).host());
    chat.setAutonomy(toolAutonomyId(options.autonomy));
    chat.setAppVersion(options.appVersion, options.gitCommit);
    const QString startProject = SupraFit::ProjectManager::instance().getCurrentProjectId();
    chat.addContextId(startProject);
    ChatRecorder recorder(&session, &registry);
    recorder.setContextArguments({ QStringLiteral("project_id") });
    recorder.setLog(&chat);
    recorder.setAutonomy(options.autonomy);
    // Below the chosen autonomy a tool asks; a headless run has nobody to ask.
    session.setApprovalPolicy(recorder.wrapApproval([](const ToolSpec& spec, const QJsonObject&) {
        fmt::print(stderr, "[refused] {} needs approval; raise the autonomy to allow it\n", spec.name.toStdString());
        return false;
    }, QStringLiteral("policy")));

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
        if (!text.trimmed().isEmpty())
            chat.append(ChatEntry::Kind::Assistant, text);
    });
    QObject::connect(&session, &LlmSession::failed, &loop, [&](const QString& e) {
        fmt::print(stderr, "\nAgent failed: {}\n", e.toStdString());
        chat.append(ChatEntry::Kind::Notice, QStringLiteral("Agent failed: ") + e);
        exitCode = 1;
        loop.quit();
    });
    QObject::connect(&session, &LlmSession::finished, &loop, &QEventLoop::quit);

    chat.append(ChatEntry::Kind::User, options.question);
    session.ask(options.question);
    if (session.isBusy())
        loop.exec();

    chat.setConversation(session.conversation());
    chat.touch();
    if (options.storeInProjects) {
        QStringList projects = chat.contextIds();
        if (projects.isEmpty())
            projects = SupraFit::ProjectManager::instance().getLoadedProjectIds();
        for (const QString& id : projects)
            ProjectChats::store(id, chat);
        if (!projects.isEmpty())
            fmt::print(stderr, "\n[agent] conversation stored in {} project(s); save them (-o) to keep it\n", projects.size());
    }
    return exitCode;
}
