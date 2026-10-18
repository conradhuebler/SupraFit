# Agent - LLM-gesteuerte agentische Auswertung

## Übersicht

Werkzeugschicht für LLM-Agenten: ein LLM (OpenAI-kompatible API, z. B. lokales
Ollama) bedient SupraFit über **benannte Werkzeuge mit typisierten Parametern und
strukturiertem Ergebnis** — nicht über generierten Code. Roadmap und Phasenplan:
[roadmap/llm_agent_analysis.md](../../../roadmap/llm_agent_analysis.md).
Vorlage: qurcuma `docs/WP-llm-tool-layer.md` + curcuma `docs/TOOL_API_WP.md`.

## Architektur (Schichten, saubere Trennung)

**Der generische Teil liegt in der gemeinsamen Bibliothek CuteChat** (`../CuteChat`, `external/CuteChat` oder `-DCUTECHAT_DIR=`;
Entwurf: [roadmap/shared_chat_library.md](../../roadmap/shared_chat_library.md)): Tool-Kontrakt, Registry, Dispatcher, `LlmClient`,
`LlmConfig`/`LlmPresets`, `LlmSession`, `ChatLog`/`ChatRecorder`/`ChatBackend`, `ChatDock`, `EndpointDialog`. Includes: `<cutechat/xyz.h>`.
Hier bleibt, was SupraFit-spezifisch ist:

| Datei | Schicht | Qt-Abhängigkeit |
|---|---|---|
| `tools_*.cpp` | Konkrete Werkzeuge (projekt, models, analysis, inspect) gegen `ProjectManager` | gemischt |
| `postfitjobs.{h,cpp}` | Job-Konfigurationen für `run_postfit_analysis` aus den Dialog-Defaults | QtCore + core |
| `agentrunner.{h,cpp}` | `suprafit_cli --agent` + Systemprompt `agentSystemPrompt()` | QtCore + core |
| `agenthelp.{h,cpp}`, `agenthelp.qrc` | Programmhilfe: `docs/ASSISTANT_HELP.md` wird eincompiliert und dem Chat als `ChatHost::helpText()` (und dem Headless-Runner) gegeben; Tool `get_help` und der Prompt-Schalter kommen aus CuteChat (`ChatHelp`) | QtCore |
| `projectchats.{h,cpp}` | `ProjectChats` + `ProjectChatBackend` (Chats im Projekt unter `agent_chats`, JSON-Tags mit Präfix `suprafit`) | QtCore + core |
| `../ui/widgets/suprafitchathost.{h,cpp}` | `SupraFitChatHost`: Prompt, Tools, Backend, Texte, Freigabe-Dialog für das `ChatDock` | Widgets |

**Prinzip:** Registry, Dispatcher und Schema-Prüfung sind GUI-frei und headless
testbar (ctest gegen `QTcpServer`-Stub, kein Netz in ctest). Erste Tools laufen
gegen `ProjectManager::instance()` — wie der CLI.

## Programmhilfe

- Jedes sichtbare Feature steht in `docs/ASSISTANT_HELP.md` (Regel in der Haupt-`CLAUDE.md`, „Where Things Go"); `AgentHelpTest` prüft nummerierte Abschnitte und dass jedes Tool dort in Backticks genannt ist.
- Zwei Wege zum Modell, beide in CuteChat: Tool `get_help` (nur bei Bedarf) oder der ganze Text im Systemprompt (Menü ☰ „Program help in the prompt", CLI `--agent-help`).

## Konventionen

- **Tool = benannte Operation**: `ToolSpec{name, description, category, paramSchema,
  effect, affinity, handler}`; Ergebnis `ToolResult{ok, data, text, image,
  truncated, error}`.
- **`ToolEffect` steuert die Freigabe:** `Read`/`Display` laufen ohne Rückfrage,
  `Mutate`/`Compute`/`FileWrite`/`Process` brauchen Zustimmung (`ToolAutonomy`:
  Ask / InProgram / Full — Operator-Entscheidung pro Sitzung).
- **`ToolAffinity::Gui`** — Dispatcher marshalts auf den GUI-Thread mit Timeout.
- **Schema-Validierung ohne externe Bibliothek** (~80 Zeilen gegen `QJsonObject`):
  type/required/enum/minimum/maximum/items(skalar), **unbekannte Schlüssel ablehnen**.
- **Kein API-Key in Konfig-Dateien** — Profil nennt nur den Namen der ENV-Variable.
- **Katalog-Kosten messen**: `ToolRegistry::catalogueBytes()`; `available`-Prädikat
  filtert kontextabhängig; `truncated` + `maxToolResultChars` kappen Ergebnisse.
- Neue Dateien: Copyright `Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>`,
  Claude Generated 2026 in Kommentaren markieren.
- Jede neue Quelle in `CMakeLists.txt` unter `if(USE_LLM)` → `add_library(agent ...)`
  eintragen. Tool-Kontrakt/Registry-Regeln stehen in `CuteChat/CLAUDE.md`.

## Build

- `USE_LLM` (default ON) schaltet `Qt6::Network` + die `agent`-Lib frei.
- `-DUSE_LLM=OFF` baut exakt wie zuvor (Option erst mit Chat-Dock tragend).

## Tests

Headless, kein Netz. Die generischen Tests (`ToolRegistryTest`, `ToolDispatcherTest`, `LlmClientTest`, `LlmSessionTest`, `ChatLogTest`,
`LlmConfigPresetTest`, `ChatDockTest`) gehören CuteChat und laufen in SupraFits ctest mit (`CUTECHAT_BUILD_TESTS`); hier: `ToolsProject/Analysis/Inspect`, `ProjectChatsTest`.

- Chats gehören ins Projekt: `ProjectChats::store()` (`ChatLog::setFormatPrefix("suprafit")` hält die Tags) → `ProjectManager`-Attachment `agent_chats`; die Kopie in `<AppData>/chats` ist nur Komfort.
- `llm.json` wird über `LlmConfig::save()` geschrieben (atomar, unbekannte Schlüssel und `_note` bleiben, nie ein Key).
- `LlmClient::setProfile()` verwirft einen Modell-Override eines anderen Profils; Listen-Antworten eines alten Profils werden verworfen.
