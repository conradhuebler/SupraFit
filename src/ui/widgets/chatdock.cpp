// chatdock.cpp - see chatdock.h
// Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
//
// Claude Generated 2026
#include "chatdock.h"

#include "src/agent/agentrunner.h"
#include "src/agent/llmclient.h"
#include "src/agent/llmconfig.h"
#include "src/agent/llmsession.h"
#include "src/agent/tooldispatcher.h"
#include "src/agent/toolregistry.h"
#include "src/agent/tools_analysis.h"
#include "src/agent/tools_project.h"
#include "src/core/projectmanager.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTextBrowser>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QVBoxLayout>

ChatDock::ChatDock(QWidget* parent)
    : QDockWidget(tr("Agent"), parent)
{
    setObjectName(QStringLiteral("chat_dock"));

    m_profile = new QComboBox;
    m_profile->setToolTip(tr("LLM endpoint profile (llm.json)"));
    m_model = new QComboBox;
    m_model->setEditable(true);
    m_model->setToolTip(tr("Model served by the endpoint (empty = profile default)"));
    m_refresh = new QPushButton(tr("Models"));
    m_refresh->setToolTip(tr("Ask the endpoint which models it offers"));
    m_autonomy = new QComboBox;
    m_autonomy->addItem(tr("Ask for fits"), int(ToolAutonomy::Ask));
    m_autonomy->addItem(tr("Run in program"), int(ToolAutonomy::InProgram));
    m_autonomy->addItem(tr("Full"), int(ToolAutonomy::Full));
    m_autonomy->setToolTip(tr("Which tool calls run without asking"));

    m_transcript = new QTextBrowser;
    m_transcript->setOpenExternalLinks(false);
    m_input = new QLineEdit;
    m_input->setPlaceholderText(tr("Ask about the open projects..."));
    m_input->setMinimumWidth(160);
    setMinimumWidth(340);
    m_send = new QPushButton(tr("Send"));
    m_stop = new QPushButton(tr("Stop"));
    m_stop->setEnabled(false);
    m_status = new QLabel;

    auto* top = new QHBoxLayout;
    top->addWidget(m_profile, 1);
    top->addWidget(m_autonomy);
    auto* modelRow = new QHBoxLayout;
    modelRow->addWidget(m_model, 1);
    modelRow->addWidget(m_refresh);
    auto* bottom = new QHBoxLayout;
    bottom->addWidget(m_input, 1);
    bottom->addWidget(m_send);
    bottom->addWidget(m_stop);
    auto* layout = new QVBoxLayout;
    layout->addLayout(top);
    layout->addLayout(modelRow);
    layout->addWidget(m_transcript, 1);
    layout->addLayout(bottom);
    layout->addWidget(m_status);
    auto* holder = new QWidget;
    holder->setLayout(layout);
    setWidget(holder);

    m_registry = new ToolRegistry;
    registerProjectTools(*m_registry, &SupraFit::ProjectManager::instance());
    registerAnalysisTools(*m_registry, &SupraFit::ProjectManager::instance());
    m_dispatcher = new ToolDispatcher(m_registry, this);
    m_client = new LlmClient(this);
    m_session = new LlmSession(m_client, m_registry, m_dispatcher, this);
    m_session->setSystemPrompt(agentSystemPrompt());
    m_session->setApprovalPolicy([this](const ToolSpec& spec, const QJsonObject& args) {
        const QString text = tr("The agent wants to run '%1'.\n\n%2\n\nAllow?")
                                 .arg(spec.name, QString::fromUtf8(QJsonDocument(args).toJson(QJsonDocument::Compact)));
        return QMessageBox::question(this, tr("Agent"), text) == QMessageBox::Yes;
    });

    connect(m_session, &LlmSession::assistantChunk, this, [this](const QString& t) {
        m_streamed = true;
        insertAnswer(t);
    });
    connect(m_session, &LlmSession::assistantMessage, this, [this](const QString& t) {
        if (!m_streamed)
            insertAnswer(t);
        m_streamed = false;
        m_answerOpen = false;
    });
    connect(m_session, &LlmSession::toolStarted, this, [this](const QString& n, const QJsonObject& a) {
        appendLine(QStringLiteral("<i style='color:gray'>&#9881; %1 %2</i>")
                       .arg(n.toHtmlEscaped(),
                            QString::fromUtf8(QJsonDocument(a).toJson(QJsonDocument::Compact)).toHtmlEscaped()));
    });
    connect(m_session, &LlmSession::toolFinished, this, [this](const QString& n, const ToolResult& r) {
        if (!r.ok)
            appendLine(QStringLiteral("<i style='color:red'>%1: %2</i>").arg(n.toHtmlEscaped(), r.error.toHtmlEscaped()));
    });
    connect(m_session, &LlmSession::toolRefused, this, [this](const QString& n, const QString& why) {
        appendLine(QStringLiteral("<i style='color:orange'>%1 refused: %2</i>").arg(n.toHtmlEscaped(), why.toHtmlEscaped()));
    });
    connect(m_session, &LlmSession::failed, this, [this](const QString& e) {
        appendLine(QStringLiteral("<b style='color:red'>%1</b>").arg(e.toHtmlEscaped()));
        setRunning(false);
    });
    connect(m_session, &LlmSession::finished, this, [this] { setRunning(false); });

    connect(m_client, &LlmClient::modelsListed, this, [this](const QStringList& models) {
        const QString keep = m_model->currentText();
        m_model->clear();
        m_model->addItems(models);
        m_model->setCurrentText(keep);
        m_status->setText(models.isEmpty() ? tr("The endpoint offers no model list") : tr("%1 models").arg(models.size()));
    });
    connect(m_refresh, &QPushButton::clicked, this, [this] {
        if (prepareClient())
            m_client->listModels();
    });
    connect(m_profile, &QComboBox::currentTextChanged, this, [this](const QString&) {
        LlmConfig config;
        LlmProfile p;
        if (config.load(LlmConfig::defaultPath()) && config.profile(m_profile->currentText(), p))
            m_model->setCurrentText(p.model);
    });
    connect(m_send, &QPushButton::clicked, this, &ChatDock::send);
    connect(m_input, &QLineEdit::returnPressed, this, &ChatDock::send);
    connect(m_stop, &QPushButton::clicked, this, &ChatDock::stop);
    reloadProfiles();
}

ChatDock::~ChatDock()
{
    m_session->cancel();
    delete m_registry;  // the dispatcher only borrows it; both go with the dock
}

void ChatDock::reloadProfiles()
{
    m_profile->clear();
    LlmConfig config;
    QString error;
    if (!config.load(LlmConfig::defaultPath(), &error)) {
        QString werr;
        LlmConfig::writeExampleIfMissing(LlmConfig::defaultPath(), &werr);
        m_status->setText(tr("No usable LLM config: %1 (example written to %2)")
                              .arg(error, LlmConfig::defaultPath()));
        return;
    }
    for (const LlmProfile& p : config.profiles())
        m_profile->addItem(p.name);
    m_profile->setCurrentText(config.activeProfileName());
    m_status->clear();
}

bool ChatDock::prepareClient()
{
    LlmConfig config;
    QString error;
    LlmProfile profile;
    if (!config.load(LlmConfig::defaultPath(), &error) || !config.profile(m_profile->currentText(), profile)) {
        appendLine(QStringLiteral("<b style='color:red'>%1</b>")
                       .arg(tr("LLM config unusable: %1").arg(error).toHtmlEscaped()));
        return false;
    }
    const QString keyProblem = LlmConfig::missingKeyMessage(profile);
    if (!keyProblem.isEmpty()) {
        appendLine(QStringLiteral("<b style='color:red'>%1</b>").arg(keyProblem.toHtmlEscaped()));
        return false;
    }
    if (!LlmConfig::isLocalEndpoint(profile))
        appendLine(QStringLiteral("<i style='color:orange'>%1</i>")
                       .arg(tr("Remote endpoint %1: the conversation and the project data the agent reads are sent there.")
                                .arg(profile.baseUrl).toHtmlEscaped()));
    m_client->setProfile(profile);
    m_client->setApiKey(LlmConfig::apiKeyFor(profile));
    if (!m_model->currentText().trimmed().isEmpty())
        m_client->setModel(m_model->currentText().trimmed());
    m_session->setMaxIterations(profile.maxToolIterations);
    m_session->setAutonomy(ToolAutonomy(m_autonomy->currentData().toInt()));
    return true;
}

void ChatDock::send()
{
    const QString question = m_input->text().trimmed();
    if (question.isEmpty() || m_session->isBusy() || !prepareClient())
        return;
    m_input->clear();
    appendLine(QStringLiteral("<b>%1</b>").arg(question.toHtmlEscaped()));
    m_streamed = false;
    setRunning(true);
    m_session->ask(question);
}

void ChatDock::stop()
{
    m_session->cancel();
    setRunning(false);
}

void ChatDock::setRunning(bool running)
{
    m_send->setEnabled(!running);
    m_input->setEnabled(!running);
    m_stop->setEnabled(running);
    m_status->setText(running ? tr("Working...") : QString());
}

void ChatDock::appendLine(const QString& html)
{
    m_transcript->moveCursor(QTextCursor::End);
    m_transcript->append(html);
    m_answerOpen = false;
}

// The answer gets its own block and the default character format, otherwise it
// inherits the bold question or the grey tool line it follows.
void ChatDock::insertAnswer(const QString& text)
{
    if (!m_answerOpen) {
        m_transcript->append(QString());
        m_answerOpen = true;
    }
    QTextCursor cursor = m_transcript->textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(text, QTextCharFormat());
    m_transcript->setTextCursor(cursor);
    m_transcript->ensureCursorVisible();
}
