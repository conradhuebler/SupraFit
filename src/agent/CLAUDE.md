# Agent - LLM-gesteuerte agentische Auswertung

## Übersicht

Werkzeugschicht für LLM-Agenten: ein LLM (OpenAI-kompatible API, z. B. lokales
Ollama) bedient SupraFit über **benannte Werkzeuge mit typisierten Parametern und
strukturiertem Ergebnis** — nicht über generierten Code. Roadmap und Phasenplan:
[roadmap/llm_agent_analysis.md](../../../roadmap/llm_agent_analysis.md).
Vorlage: qurcuma `docs/WP-llm-tool-layer.md` + curcuma `docs/TOOL_API_WP.md`.

## Architektur (Schichten, saubere Trennung)

| Datei | Schicht | Qt-Abhängigkeit |
|---|---|---|
| `tool.h/.cpp` | Tool-Kontrakt (Werttypen: `ToolSpec`, `ToolResult`, `ToolEffect`, `ToolAffinity`) | QtCore only |
| `toolregistry.h/.cpp` | Katalog + Schema-Validierung + `invoke()` | QtCore only |
| `tooldispatcher.h/.cpp` | Thread-Marshalling (GUI-Thread) + Audit-Spur | QtCore only |
| `llmconfig.h/.cpp` | Profile aus `~/.config/suprafit/llm.json`, Key nur aus ENV | QtCore only |
| `llmclient.h/.cpp` | OpenAI-kompatibel, SSE-Streaming, Tool-Calls | QtCore + QtNetwork |
| `llmsession.h/.cpp` | Agent-Loop: senden → Tool ausführen → Ergebnis zurück | QtCore + QtNetwork |
| `tools_*.cpp` | Konkrete Werkzeuge (projekt, models, analysis, io) | gemischt |
| `chatlog.h/.cpp` | Sichtbarer Chat als Daten: Einträge, JSON/Markdown-Export+Import, Verlauf (`<AppData>/chats`, Kappe 50), Eingabe-Historie | QtCore only |

**Prinzip:** Registry, Dispatcher und Schema-Prüfung sind GUI-frei und headless
testbar (ctest gegen `QTcpServer`-Stub, kein Netz in ctest). Erste Tools laufen
gegen `ProjectManager::instance()` — wie der CLI.

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
  eintragen.

## Build

- `USE_LLM` (default ON) schaltet `Qt6::Network` + die `agent`-Lib frei.
- `-DUSE_LLM=OFF` baut exakt wie zuvor (Option erst mit Chat-Dock tragend).

## Tests

Headless, kein Netz: `test_toolregistry`, `test_tooldispatcher`, `test_llmclient`
(gegen `QTcpServer`-Stub), `test_llmsession` (Stub mit Antwort-Folge), `test_chatlog`.
