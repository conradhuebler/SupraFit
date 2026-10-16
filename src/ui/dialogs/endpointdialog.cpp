// endpointdialog.cpp - see endpointdialog.h
// Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
//
// Claude Generated 2026 (UI redesign WP10).
#include "endpointdialog.h"

#include "src/agent/llmclient.h"
#include "src/agent/llmpresets.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QPushButton>
#include <QSpinBox>
#include <QToolButton>
#include <QVBoxLayout>

#include <memory>

namespace SessionKeys {
namespace {
    QHash<QString, QString>& store()
    {
        static QHash<QString, QString> keys;
        return keys;
    }
}
QString key(const QString& profileName)
{
    return store().value(profileName);
}
void setKey(const QString& profileName, const QString& key)
{
    if (key.isEmpty())
        store().remove(profileName);
    else
        store().insert(profileName, key);
}
QString effectiveKey(const LlmProfile& profile)
{
    const QString env = LlmConfig::apiKeyFor(profile);
    return env.isEmpty() ? key(profile.name) : env;
}
} // namespace SessionKeys

namespace {
QLabel* hint(const QString& text)
{
    auto* label = new QLabel(text);
    label->setWordWrap(true);
    label->setForegroundRole(QPalette::PlaceholderText);
    return label;
}
} // namespace

EndpointDialog::EndpointDialog(const QString& configPath, QWidget* parent)
    : QDialog(parent)
    , m_path(configPath.isEmpty() ? LlmConfig::defaultPath() : configPath)
{
    setWindowTitle(tr("LLM endpoints"));
    QString error;
    if (!m_config.load(m_path, &error)) {
        // no (usable) file yet: start from the default Ollama profile; written on Save
        m_config = LlmConfig::defaultConfig();
    }
    m_profiles = m_config.profiles();
    m_active = m_config.activeProfileName();
    m_client = new LlmClient(this);

    // ---- left: profiles ----
    m_list = new QListWidget;
    m_list->setMinimumWidth(170);
    auto* add = new QToolButton;
    add->setText(tr("Add from preset"));
    add->setPopupMode(QToolButton::InstantPopup);
    add->setToolButtonStyle(Qt::ToolButtonTextOnly);
    auto* presetMenu = new QMenu(add);
    presetMenu->setToolTipsVisible(true);
    for (const LlmPreset& preset : LlmPresets::catalog()) {
        QAction* action = presetMenu->addAction(preset.name + (preset.local ? tr("  (this computer)") : QString()));
        action->setToolTip(preset.note);
        const QString id = preset.id;
        connect(action, &QAction::triggered, this, [this, id]() { addFromPreset(id); });
    }
    add->setMenu(presetMenu);
    add->setToolTip(tr("New profile from a provider preset (Ollama, LM Studio, OpenAI, Gemini, ...)"));
    m_duplicate = new QPushButton(tr("Duplicate"));
    m_delete = new QPushButton(tr("Delete"));
    m_setActive = new QPushButton(tr("Set active"));
    m_setActive->setToolTip(tr("The active profile is used by the chat and by suprafit_cli without --profile"));
    auto* listButtons = new QVBoxLayout;
    listButtons->addWidget(add);
    listButtons->addWidget(m_duplicate);
    listButtons->addWidget(m_delete);
    listButtons->addWidget(m_setActive);
    auto* left = new QVBoxLayout;
    left->addWidget(m_list, 1);
    left->addLayout(listButtons);

    // ---- right: the selected profile ----
    m_name = new QLineEdit;
    m_preset = new QLabel;
    m_baseUrl = new QLineEdit;
    m_baseUrl->setPlaceholderText(QStringLiteral("http://127.0.0.1:11434/v1"));
    m_keyEnv = new QLineEdit;
    m_keyEnv->setPlaceholderText(tr("e.g. OPENAI_API_KEY (empty for local servers)"));
    m_keyStatus = new QLabel;
    m_keyStatus->setWordWrap(true);
    m_sessionKey = new QLineEdit;
    m_sessionKey->setEchoMode(QLineEdit::Password);
    m_sessionKey->setPlaceholderText(tr("only for this session, never saved"));
    m_sessionKey->setToolTip(tr("A key typed here stays in memory until SupraFit ends and is never written to any file"));
    m_model = new QLineEdit;
    m_model->setPlaceholderText(tr("empty: chosen from the endpoint's list"));
    m_stream = new QCheckBox(tr("Stream the answer"));
    m_reasoning = new QComboBox;
    m_reasoning->addItem(tr("none"), QString());
    m_reasoning->addItem(tr("think (Ollama)"), QStringLiteral("think"));
    m_reasoning->addItem(tr("reasoning_effort (OpenAI o-series/gpt-5, Gemini)"), QStringLiteral("reasoning_effort"));
    m_reasoning->setToolTip(tr("Field that asks the model to reason before answering; servers that do not know it may reject the request"));
    m_timeout = new QSpinBox;
    m_timeout->setRange(10, 3600);
    m_timeout->setSuffix(tr(" s"));
    m_iterations = new QSpinBox;
    m_iterations->setRange(1, 500);
    m_iterations->setToolTip(tr("Maximum tool rounds per question"));
    m_note = hint(QString());
    m_test = new QPushButton(tr("Test connection"));
    m_test->setToolTip(tr("Ask the endpoint for its model list (GET <base URL>/models)"));
    m_testResult = new QLabel;
    m_testResult->setWordWrap(true);
    m_testResult->setTextInteractionFlags(Qt::TextSelectableByMouse);

    auto* form = new QFormLayout;
    form->setRowWrapPolicy(QFormLayout::WrapLongRows);
    form->addRow(tr("Name"), m_name);
    form->addRow(tr("Preset"), m_preset);
    form->addRow(tr("Base URL"), m_baseUrl);
    form->addRow(tr("Key variable"), m_keyEnv);
    form->addRow(tr("Session key"), m_sessionKey);
    form->addRow(tr("Default model"), m_model);
    form->addRow(QString(), m_stream);
    form->addRow(tr("Reasoning field"), m_reasoning);
    form->addRow(tr("Timeout"), m_timeout);
    form->addRow(tr("Tool rounds"), m_iterations);
    auto* testRow = new QHBoxLayout;
    testRow->addWidget(m_test);
    testRow->addWidget(m_testResult, 1);
    auto* right = new QVBoxLayout;
    right->addLayout(form);
    right->addWidget(m_keyStatus);
    right->addWidget(m_note);
    right->addLayout(testRow);
    right->addStretch();

    m_message = new QLabel;
    m_message->setWordWrap(true);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &EndpointDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &EndpointDialog::reject);

    auto* columns = new QHBoxLayout;
    columns->addLayout(left, 2);
    columns->addLayout(right, 5);
    auto* layout = new QVBoxLayout;
    layout->addLayout(columns, 1);
    layout->addWidget(hint(tr("Saved in %1. The file never holds a key; hosted providers read it from the named environment variable.").arg(m_path)));
    layout->addWidget(m_message);
    layout->addWidget(buttons);
    setLayout(layout);
    resize(820, 560);

    connect(m_list, &QListWidget::currentRowChanged, this, [this](int row) {
        storeFields();
        showProfile(row);
    });
    for (QLineEdit* edit : { m_name, m_baseUrl, m_keyEnv, m_model })
        connect(edit, &QLineEdit::editingFinished, this, [this]() {
            storeFields();
            rebuildList();
        });
    connect(m_keyEnv, &QLineEdit::textChanged, this, [this]() { updateKeyStatus(); });
    connect(m_sessionKey, &QLineEdit::textChanged, this, [this](const QString& key) {
        if (m_row >= 0 && m_row < m_profiles.size())
            SessionKeys::setKey(m_profiles[m_row].name, key);
        updateKeyStatus();
    });
    connect(m_duplicate, &QPushButton::clicked, this, [this]() {
        storeFields();
        if (m_row < 0)
            return;
        LlmProfile copy = m_profiles[m_row];
        copy.name = uniqueName(copy.name);
        m_profiles.insert(m_row + 1, copy);
        rebuildList();
        m_list->setCurrentRow(m_row + 1);
    });
    connect(m_delete, &QPushButton::clicked, this, [this]() {
        if (m_row < 0 || m_profiles.size() <= 1) {
            m_message->setText(tr("At least one profile is needed."));
            return;
        }
        const QString name = m_profiles[m_row].name;
        m_profiles.removeAt(m_row);
        if (m_active == name)
            m_active = m_profiles.first().name;
        m_row = -1;
        rebuildList();
        m_list->setCurrentRow(0);
    });
    connect(m_setActive, &QPushButton::clicked, this, [this]() {
        storeFields();
        if (m_row >= 0) {
            m_active = m_profiles[m_row].name;
            rebuildList();
        }
    });
    connect(m_test, &QPushButton::clicked, this, &EndpointDialog::testConnection);

    rebuildList();
    int activeRow = 0;
    for (int i = 0; i < m_profiles.size(); ++i)
        if (m_profiles[i].name == m_active)
            activeRow = i;
    m_list->setCurrentRow(activeRow);
}

QString EndpointDialog::uniqueName(const QString& base) const
{
    auto taken = [this](const QString& name) {
        for (const LlmProfile& p : m_profiles)
            if (p.name == name)
                return true;
        return false;
    };
    if (!taken(base))
        return base;
    for (int i = 2;; ++i) {
        const QString name = QStringLiteral("%1 %2").arg(base).arg(i);
        if (!taken(name))
            return name;
    }
}

void EndpointDialog::rebuildList()
{
    const int row = m_list->currentRow();
    m_loading = true;
    m_list->clear();
    for (const LlmProfile& p : m_profiles) {
        auto* item = new QListWidgetItem((p.name == m_active ? QStringLiteral("★ ") : QStringLiteral("   ")) + p.name);
        item->setToolTip(p.baseUrl);
        m_list->addItem(item);
    }
    m_loading = false;
    if (row >= 0 && row < m_list->count())
        m_list->setCurrentRow(row);
}

void EndpointDialog::showProfile(int row)
{
    if (m_loading)
        return;
    m_row = row;
    const bool valid = row >= 0 && row < m_profiles.size();
    for (QWidget* w : std::initializer_list<QWidget*>{ m_name, m_baseUrl, m_keyEnv, m_sessionKey, m_model, m_stream, m_reasoning, m_timeout, m_iterations, m_test, m_duplicate, m_delete, m_setActive })
        w->setEnabled(valid);
    if (!valid)
        return;
    const LlmProfile& p = m_profiles[row];
    m_loading = true;
    m_name->setText(p.name);
    const LlmPreset* preset = LlmPresets::find(p.preset);
    m_preset->setText(preset ? preset->name : tr("(none)"));
    m_note->setText(preset ? preset->note : QString());
    m_baseUrl->setText(p.baseUrl);
    m_keyEnv->setText(p.apiKeyEnv);
    m_sessionKey->setText(SessionKeys::key(p.name));
    m_model->setText(p.model);
    m_stream->setChecked(p.stream);
    const int reasoning = m_reasoning->findData(p.reasoningField);
    if (reasoning >= 0)
        m_reasoning->setCurrentIndex(reasoning);
    else {
        m_reasoning->addItem(p.reasoningField, p.reasoningField);
        m_reasoning->setCurrentIndex(m_reasoning->count() - 1);
    }
    m_timeout->setValue(p.requestTimeoutMs / 1000);
    m_iterations->setValue(p.maxToolIterations);
    m_testResult->clear();
    m_loading = false;
    updateKeyStatus();
}

void EndpointDialog::storeFields()
{
    if (m_loading || m_row < 0 || m_row >= m_profiles.size())
        return;
    LlmProfile& p = m_profiles[m_row];
    const QString oldName = p.name;
    p.name = m_name->text().trimmed();
    if (oldName == m_active)
        m_active = p.name;
    if (oldName != p.name && !SessionKeys::key(oldName).isEmpty()) {
        SessionKeys::setKey(p.name, SessionKeys::key(oldName));
        SessionKeys::setKey(oldName, QString());
    }
    p.baseUrl = m_baseUrl->text().trimmed();
    p.apiKeyEnv = m_keyEnv->text().trimmed();
    p.model = m_model->text().trimmed();
    p.stream = m_stream->isChecked();
    p.reasoningField = m_reasoning->currentData().toString();
    p.requestTimeoutMs = m_timeout->value() * 1000;
    p.maxToolIterations = m_iterations->value();
}

void EndpointDialog::addFromPreset(const QString& presetId)
{
    storeFields();
    const LlmPreset* preset = LlmPresets::find(presetId);
    if (!preset)
        return;
    m_profiles.append(preset->toProfile(uniqueName(preset->name)));
    rebuildList();
    m_list->setCurrentRow(m_profiles.size() - 1);
}

void EndpointDialog::updateKeyStatus()
{
    const QString env = m_keyEnv->text().trimmed();
    if (env.isEmpty()) {
        m_keyStatus->setText(tr("No key variable: none is sent (typical for servers on this computer)."));
        m_keyStatus->setForegroundRole(QPalette::PlaceholderText);
        return;
    }
    LlmProfile probe;
    probe.apiKeyEnv = env;
    if (!LlmConfig::apiKeyFor(probe).isEmpty())
        m_keyStatus->setText(tr("✓ %1 is set in the environment.").arg(env));
    else if (!m_sessionKey->text().isEmpty())
        m_keyStatus->setText(tr("%1 is not set; the session key is used until SupraFit ends.").arg(env));
    else
        m_keyStatus->setText(tr("✗ %1 is not set. Set it before starting SupraFit, e.g. in ~/.profile: export %1=<your key>, or enter a session key in the field above.").arg(env));
    m_keyStatus->setForegroundRole(QPalette::WindowText);
}

void EndpointDialog::selectProfile(int row)
{
    m_list->setCurrentRow(row);
}

void EndpointDialog::testConnection()
{
    storeFields();
    if (m_row < 0)
        return;
    const LlmProfile profile = m_profiles[m_row];
    if (!LlmConfig::missingKeyMessage(profile).isEmpty() && SessionKeys::key(profile.name).isEmpty()) {
        m_testResult->setText(tr("✗ No key: %1 is not set.").arg(profile.apiKeyEnv));
        return;
    }
    m_client->disconnect(this);
    m_client->setProfile(profile);
    m_client->setApiKey(SessionKeys::effectiveKey(profile));
    m_testResult->setText(tr("Asking %1 …").arg(profile.baseUrl));
    m_test->setEnabled(false);
    auto timer = std::make_shared<QElapsedTimer>();
    timer->start();
    auto failure = std::make_shared<QString>();
    connect(m_client, &LlmClient::modelListFailed, this, [failure](const QString& e) { *failure = e; });
    connect(m_client, &LlmClient::modelsListed, this, [this, timer, failure](const QStringList& models) {
        m_test->setEnabled(true);
        if (!failure->isEmpty())
            m_testResult->setText(tr("✗ %1").arg(*failure));
        else if (models.isEmpty())
            m_testResult->setText(tr("✓ Connected in %1 ms, but the endpoint lists no models; enter the model name by hand.").arg(timer->elapsed()));
        else
            m_testResult->setText(tr("✓ %1 models in %2 ms (e.g. %3)").arg(models.size()).arg(timer->elapsed()).arg(models.mid(0, 3).join(QStringLiteral(", "))));
    });
    m_client->listModels();
}

void EndpointDialog::accept()
{
    storeFields();
    QStringList names;
    for (const LlmProfile& p : m_profiles) {
        if (p.name.isEmpty() || p.baseUrl.isEmpty()) {
            m_message->setText(tr("Every profile needs a name and a base URL."));
            return;
        }
        if (names.contains(p.name)) {
            m_message->setText(tr("The profile name '%1' is used twice.").arg(p.name));
            return;
        }
        names << p.name;
    }
    m_config.setProfiles(m_profiles);
    m_config.setActiveProfileName(names.contains(m_active) ? m_active : names.first());
    QString error;
    if (!m_config.save(m_path, &error)) {
        m_message->setText(tr("Not saved: %1").arg(error));
        return;
    }
    QDialog::accept();
}
