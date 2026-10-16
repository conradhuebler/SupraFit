// endpointdialog.h - Endpoint manager for the LLM profiles (llm.json).
// Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
//
// Claude Generated 2026 (UI redesign WP10). Lists the profiles of llm.json, adds new ones from
// the provider presets, edits, duplicates and deletes them, sets the active one and tests the
// connection (GET <base_url>/models). Keys are never written: a profile names the environment
// variable; a key typed here lives only in memory for this session (SessionKeys).
#pragma once

#include <QDialog>
#include <QHash>
#include <QString>
#include <QVector>

#include "src/agent/llmconfig.h"

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QSpinBox;
class LlmClient;

/// Keys typed in the endpoint manager, per profile name; memory only, gone when SupraFit ends.
namespace SessionKeys {
QString key(const QString& profileName);
void setKey(const QString& profileName, const QString& key);
/// Environment key of @p profile, else the session key.
QString effectiveKey(const LlmProfile& profile);
}

class EndpointDialog : public QDialog {
public:
    explicit EndpointDialog(const QString& configPath, QWidget* parent = nullptr);

    /// Select the profile at @p row and start a connection test (also used by the capture tooling).
    void selectProfile(int row);
    void testConnection();
    QString savedPath() const { return m_path; }

protected:
    void accept() override;

private:
    void rebuildList();
    void showProfile(int row);
    void storeFields();
    void addFromPreset(const QString& presetId);
    void updateKeyStatus();
    QString uniqueName(const QString& base) const;

    QString m_path;
    LlmConfig m_config;
    QVector<LlmProfile> m_profiles;
    QString m_active;
    int m_row = -1;
    bool m_loading = false;

    QListWidget* m_list;
    QLineEdit *m_name, *m_baseUrl, *m_keyEnv, *m_sessionKey, *m_model;
    QLabel *m_preset, *m_note, *m_keyStatus, *m_testResult, *m_message;
    QCheckBox* m_stream;
    QComboBox* m_reasoning;
    QSpinBox *m_timeout, *m_iterations;
    QPushButton *m_duplicate, *m_delete, *m_setActive, *m_test;
    LlmClient* m_client;
};
