<!--
Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
Claude Generated - work package / roadmap document (no production code changed)
-->

# Work Package: Agentische Auswertung — LLM-gesteuerte Analyse in SupraFit

**Status:** WIP (Phase 0+1+2+3 done, Phase 4 next) · **Owner:** Conrad Hübler · **Author note:** Claude Generated
**Branch:** `feature/llm-agent-analysis` · **Vorlage:** qurcuma `feature/llm-tools` (`docs/WP-llm-tool-layer.md`) + curcuma `llm-core` (`docs/TOOL_API_WP.md`)

## Context

SupraFit ist ein JSON-getriebenes C++/Qt6-Framework: Projekte, Modelle, Fits und die
gesamte Statistik laufen über `QJsonObject`. `ProjectManager` ist ein thread-sicherer
Singleton und funktioniert headless (der CLI beweist es). Was fehlt, ist eine
**parametrisierte Aufrufebene**, die ein LLM bedienen kann — analog zu qurcumas
Tool-Registry, wo der Umbau von „alles hängt an Widget-Slots" in „benannte Operation
mit typisierten Parametern und strukturiertem Ergebnis" der Kern der Arbeit war.

Endpunktseitig **OpenAI-kompatibel**, testweise gegen die lokale Ollama-Installation
(`http://localhost:11434/v1`). Qurcuma hat gegen denselben Endpunkt gemessen
(`glm-5.3-flash:cloud`); lokal verfügbare tool-fähige Modelle: `ornith-1.5:9b`,
`gpt-oss:20b`, `lfm2.5-thinking:latest`.

## Goal

Ein LLM soll SupraFit-Projekte agentisch auswerten können: Projekte und Modelle
inventarisieren, Fits auslösen, Post-Fit-Statistik (Monte Carlo, Cross-Validation,
Model Comparison, Reduction) fahren, Modelle per AIC vergleichen, Ergebnisse
zusammenfassen — gesteuert über Werkzeuge, nicht über generierten Code.

Leitszenario (Prüfstein statt Featureliste): **„Lade Projekt X, fitte alle Modelle,
vergleiche sie per AIC und Monte Carlo, sage mir welches Modell die Daten trägt und
warum."** Zu jeder Handlung muss es eine Beobachtung geben — ein Modell, das fitten
darf, aber nicht lesen kann, ob es konvergiert ist, rät.

Non-Goals (erste Ausbaustufe): keine GUI-Chat-Oberfläche, kein Schreiben in
Projekte, kein Datenimport aus Fremddateien.

## Current State

### Was existiert und werkzeugförmig ist (wiederverwendet wird)

| Capability | API | Fundstelle | Effektklasse |
|---|---|---|---|
| Projekt laden | `ProjectManager::loadProject(path)` | `src/core/projectmanager.h:84` | FileWrite |
| Projekt-Inventar | `getLoadedProjectIds()`, `getProjectModels()`, `getModelIds()` | `:141-237` | Read |
| Projekt-Info | `getProjectData()`, `getProjectDisplayInfo()` | `:128, :246` | Read |
| Modell erstellen | `CreateModel(int id, data)` / `CreateModel(QJsonObject, data)` | `src/core/models/models.h:34-36` | Mutate |
| Modell fitten | `model->Calculate()` | `src/core/models/AbstractModel.h:913` | Compute |
| Fit-Statistik | `SSE()`, `GetAIC()`, `isConverged()`, `GlobalTable()/LocalTable()` | `AbstractModel.h` | Read |
| AIC-Vergleich | `StatisticTool::CalculateAICMetrics(models)` → JSON | `src/core/analyse.h:70` | Read |
| Monte-Carlo | `StatisticTool::CalculateMCMetrics(...)` → JSON | `:85` | Read |
| Cross-Validation | `StatisticTool::CalculateCVMetrics(...)` → JSON | `:78` | Read |
| Parameter-Reduktion | `StatisticTool::CalculateReductionMetrics(...)` → JSON | `:93` | Read |
| Mehrere Modelle fitten | `AnalysisManager::fitModelsToData(data, config)` | `src/core/analysis_manager.h:87` | Compute |
| Post-Fit-Jobs (alle 7) | `AnalysisManager::executePostFitAnalysis()` / `runPostFitAnalysis()` | `:109-143` | Compute |
| JobManager (MC/WGS/MOC/CV/…) | `JobManager::AddSingleJob()` + `RunJobs()` | `src/capabilities/jobmanager.h:229` | Compute |
| Projekt speichern | `ProjectManager::saveProject()`, `getProjectAsJson()` | `projectmanager.h:92-186` | FileWrite |
| Modellkatalog | `SupraFit::Model` enum + `MODEL_ID_REFERENCE.md` | `src/global.h:156` | Read |
| BC50 | `BC50::*` Funktionen | `src/core/bc50.h` | Read |

### Was fehlt (das ist der Umbau, nicht das Chatfenster)

- **Keine Tool-Registry / kein Tool-Kontrakt.** Es gibt keine zentrale, typisierte
  Aufrufebene — die Qurcuma-Registry (`core/tool.h` + `core/toolregistry.h`) ist
  QtCore-only und headless testbar; das ist das Vorbild.
- **Kein `Qt6::Network` im Build.** `CMakeLists.txt` hat nur Core/Qml/Widgets/Charts.
  Qurcuma löst das mit `option(USE_LLM ... ON)` + einer eigenen `qurcuma_llm`-Lib.
- **Kein LLM-Client.** Qurcumas `LlmClient` (OpenAI-kompatibel, SSE, Tool-Calls,
  `POST /v1/chat/completions`) ist die Vorlage — inkl. der Ollama-Quirks (`content`
  ist `""` nicht `null`, `arguments` ist ein JSON-String, `reasoning` nicht
  `reasoning_content`).
- **Keine Agent-Loop.** Qurcumas `LlmSession` (Senden → Tools ausführen → Ergebnis
  zurück, Iterationslimit, Freigabepolitik) ist die Vorlage.
- **Kein strukturierter Log-Ringpuffer.** Heute nur `qDebug`/`fmt`. Qurcumas
  `LogHub` (QtCore-only, `qInstallMessageHandler`) ist optional für Phase 0.
- **Keine Freigabepolitik.** In einem wissenschaftlichen Werkzeug, in dem ein Modell
  Fits auslösen und Projekte verändern kann, ist „was hat es mit meinen Daten
  gemacht" eine Reproduzierbarkeitsfrage.

### Qurcuma-Prinzipien, die übernommen werden

1. **Tool = benannte Operation mit typisiertem Schema und strukturiertem Ergebnis.**
   `ToolSpec{name, description, category, paramSchema, effect, affinity, handler}`,
   `ToolResult{ok, data, text, image, truncated, error}`.
2. **`ToolEffect` steuert die Freigabe:** `Read`/`Display` laufen ohne Rückfrage,
   `Mutate`/`Compute`/`FileWrite`/`Process` brauchen Zustimmung. Dreistufige
   Autonomie (`Ask` / `InProgram` / `Full`), vom Operator gesetzt.
3. **`ToolAffinity::Gui`** — Dispatcher marshalts GUI-Zugriffe auf den GUI-Thread
   mit Timeout. In SupraFit sind Fits rechenintensiv und müssen ohnehin off-thread
   laufen; die GUI-Kopplung ist schwächer als in Qurcuma (ProjectManager ist headless).
4. **Registry ist GUI-frei und headless testbar** — ctest gegen `QTcpServer`-Stub,
   **kein Netz in ctest**.
5. **Kein API-Key in Konfig-Dateien** — Profil nennt nur den Namen der
   Umgebungsvariable (`~/.config/suprafit/llm.json`).
6. **Katalog-Kosten messen.** Qurcuma gemessen: 38 Tools = 24 kB JSON *pro Request*;
   `available`-Prädikat filtert kontextabhängig (21 statt 38 Tools = 1/3 weniger).
   `ToolRegistry::catalogueBytes()` liefert die Zahl in den Start-Log.
7. **`-DUSE_LLM=OFF` baut vollständig** — die Option wird erst mit dem Chat-Dock
   (Phase 7) tragend.
8. **Ergebnis-Kappung.** `truncated` in `ToolResult`; `maxToolResultChars` in der
   Session — History wird bei jeder Runde resendet, ein fettes Ergebnis wird immer
   wieder bezahlt.

## Architektur-Vorschlag

Neue Top-Level-Dir `src/agent/` (analog qurcuma `src/llm/` + `src/core/`):

```
src/agent/
├── tool.h / tool.cpp            ← ToolSpec, ToolResult, ToolEffect, ToolAffinity (QtCore only)
├── toolregistry.h / .cpp        ← Katalog + Schema-Validierung (QtCore only, ~80 Zeilen)
├── tooldispatcher.h / .cpp      ← Thread-Marshalling + Audit (QtCore only)
├── llmconfig.h / .cpp           ← Profile aus ~/.config/suprafit/llm.json, Key nur aus ENV
├── llmclient.h / .cpp           ← OpenAI-kompatibel, SSE, Tool-Calls (QtCore + QtNetwork)
├── llmsession.h / .cpp          ← Agent-Loop (QtCore + QtNetwork)
├── tools_project.cpp            ← list_projects, get_project_info, load_project
├── tools_models.cpp             ← list_models, get_model_fit, get_model_parameters, describe_model_catalog
├── tools_analysis.cpp           ← fit_model, fit_models, compare_models_aic, run_postfit_analysis, calculate_bc50
└── tools_io.cpp                 ← save_project, export_results
```

CMake: `option(USE_LLM "Enable LLM agent support (needs Qt6::Network)" ON)` +
eigene `agent`-Bibliothek (Core + Network). Erste Tools laufen headless gegen
`ProjectManager::instance()` — genau wie der CLI.

## Phased Roadmap

Eine Phase = ein baubarer Commit, Build grün nach jedem Schritt.

| Phase | Inhalt | Fertig, wenn |
|---|---|---|
| **0** | `USE_LLM`-Option, `Qt6::Network`, `agent`-Lib anlegen | ✅ **DONE 2026-10-11** — Build grün mit ON und OFF, ctest identisch zum Baseline |
| **1** | Tool-Kontrakt + Registry + Dispatcher (QtCore-only, headless) | ✅ **DONE 2026-10-11** — `test_toolregistry` 21/21, `test_tooldispatcher` 10/10 |
| **2** | `LlmConfig` + `LlmClient` (OpenAI-kompatibel, SSE) | ✅ **DONE 2026-10-11** — `test_llmclient` 18/18 gegen `QTcpServer`-Stub, kein Netz; 4 Bugs gefixt (siehe AIChangelog) |
| **3** | `LlmSession` Agent-Loop + Freigabepolitik | ✅ **DONE 2026-10-12** — `test_llmsession` 17/17 gegen Stub; 3 Bugs gefixt (siehe AIChangelog) |
| **4** | Read-Tools: `list_projects`, `get_project_info`, `list_models`, `get_model_fit`, `describe_model_catalog` | headless gegen echtes Projekt-JSON |
| **5** | Compute-Tools: `fit_model`, `fit_models`, `compare_models_aic`, `run_postfit_analysis`, `calculate_bc50` | ctest + Operator-Check |
| **6** | CLI-Integration: `suprafit_cli --agent "Frage"` headless | Operator-Check |
| **7** | GUI: Chat-Dock (später, eigene Phase) | — |

### Phase-Details

- **Phase 0 (0.5 day).** ✅ **DONE 2026-10-11.** `option(USE_LLM ... ON)`;
  `find_package(Qt6 6.5 COMPONENTS Network)`; `add_library(agent ...)` mit
  Core+Network, gelinkt in `suprafit`/`suprafit_cli` nur bei ON;
  `#cmakedefine USE_LLM` in `global_config.h.in`. Skeleton-Quellen angelegt:
  `tool`, `toolregistry`, `tooldispatcher`, `llmconfig`, `llmclient`, `llmsession`
  (compiliert, noch keine registrierten Tools, noch keine agent-Tests).
- **Phase 1 (2-3 days).** ✅ **DONE 2026-10-11.** `tool.h` (Werttypen), `toolregistry`
  (Katalog, Schema-Validierung: type/required/enum/min/max, **unbekannte Schlüssel
  ablehnen**), `tooldispatcher` (GUI-Marshalling mit `QSemaphore`-Timeout in
  50-ms-Polling-Slices, Interrupt-Flag, Audit-Signal, thread-lokale Origin).
  Alles QtCore-only, headless getestet: `test_toolregistry` (21 checks),
  `test_tooldispatcher` (10 checks) — beide grün und hinter `if(USE_LLM)` in
  `src/tests/CMakeLists.txt` registriert.
  **Gefundener Bug:** `requestInterrupt()` wurde während eines marshalled Waits
  nicht beachtet (einzelnes `tryAcquire` mit vollem Timeout blockierte bis zum
  Ablauf) — jetzt 50-ms-Polling gegen die Deadline, Interrupt wird innerhalb von
  50 ms erkannt.
- **Phase 2 (1-2 days).** ✅ **DONE 2026-10-11.** `llmconfig` (Profile aus
  `~/.config/suprafit/llm.json`, niemals QSettings, Key nur als ENV-Name,
  **Key im File/profile refused**, unvollständige Profile refused,
  Example-Datei ist parsebares JSON), `llmclient` (`POST /v1/chat/completions`,
  Tool-Calls in beide Richtungen, SSE-Streaming, Abbruch, Klartext-Fehler).
  Test gegen `QTcpServer`-Stub (`src/tests/test_stubserver.h`, kein Netz):
  Anfrageform, `Authorization` nur bei gesetztem Schlüssel,
  `tool_calls`-Antwort, Endpoint-Error bevorzugt vor HTTP-Status,
  HTTP-Error mit Slice, kaputtes JSON, leeres `choices`, SSE-Zerlegung,
  fragmentierter Tool-Call joined by index, reasoning nicht zurückgesendet.
  **4 Bugs gefunden und gefixt** (Config-Key/vollständige Profile/Example-JSON/
  doppeltes readAll). **moc-Trap dokumentiert** (siehe `test_llmclient.cpp`):
  Qt-6.11-moc verliert die Klasse bei Raw-Strings mit `\"` oder mehrzeiligem
  Raw-String mit `//` — Payloads mit Escapes als plain string concatenation.
- **Phase 3 (2 days).** ✅ **DONE 2026-10-12.** `llmsession`: Gesprächsverlauf,
  Katalog im OpenAI-Format (gefiltert durch `available`), Schleife *senden → Tool
  ausführen → Ergebnis zurück*, Freigabepolitik über `ToolEffect` +
  `ToolAutonomy` (ohne Rückfrage-Haken ablehnen, nicht durchwinken;
  `InProgram` lässt Compute ohne Frage laufen), Iterationslimit + finaler
  tool-loser Abschluss (jeder `tool_call` in der History wird beantwortet),
  Kappung der Tool-Ergebnisse.
  Test gegen Stub mit Antwort-Folge durch den ECHTEN `LlmClient` (kein Fake —
  die Naht client↔session ist der Punkt). **3 Bugs gefunden und gefixt:**
  (1) malformed `arguments` lieferte still leeres Objekt → Handler lief trotzdem;
  jetzt refused vor Dispatch mit Meldung + `tool_call_id` in History.
  (2) `endTurn()` clearte `requestInterrupt()` → Flag-Leak zwischen Turns;
  jetzt `ask()` clearet bei frischem Turn, `runToolCalls` prüft den Flag.
  (3) unknown-tool/refused/malformed bypasten den Dispatcher → Audit unsichtbar;
  jetzt `ToolDispatcher::recordRefusal()` zeichnet Refusals im selben
  `callRecorded`-Signal — Audit zeigt Attempts, nicht nur Erfolge.
- **Phase 4 (2 days).** Fünf Read-Tools gegen `ProjectManager::instance()`.
  `describe_model_catalog` liefert die Modell-ID-Tabelle aus
  `MODEL_ID_REFERENCE.md`/`SupraFit::Model` — das spart dem Modell, 40 IDs raten
  zu müssen.
- **Phase 5 (3-4 days).** Compute-Tools. `fit_model` ist `Effect::Compute` —
  **immer mit Freigabe**. `compare_models_aic` und `run_postfit_analysis` sind
  `Read` (lesen nur Fit-Ergebnisse) bzw. `Compute` (starten Statistik-Jobs).
  `calculate_bc50` ist `Read` (rechnet aus vorhandenen Parametern).
- **Phase 6 (1-2 days).** `suprafit_cli --agent "Frage" [--profile local]`:
  headless, ohne GUI, gegen Ollama. Systemprompt + Freigabe = `InProgram`
  (alles im Prozess bleibt ohne Rückfrage, FileWrite fragt).
- **Phase 7 (später).** Chat-Dock in der GUI, analog qurcuma `ChatDock`.

## Erste Tool-Liste (MVP)

**Read (ohne Freigabe):**
- `list_projects` — geladene Projekte mit IDs/Titeln/Modellanzahl
- `get_project_info(project_id)` — Datensätze, Reihen, Spalten, Modelle
- `list_models(project_id)` — Modell-IDs, Namen, Konvergenz-Status
- `get_model_fit(project_id, model_id)` — SSE, AIC, AICc, R², konvergiert?
- `get_model_parameters(project_id, model_id)` — globale + lokale Parameter
- `compare_models_aic(project_id, model_ids?)` — AIC-Ranking mit Evidence Ratios
- `describe_model_catalog` — verfügbare Modell-IDs mit Beschreibungen

**Compute (mit Freigabe):**
- `fit_model(project_id, model_id)` — `Calculate()` auslösen, Ergebnis-JSON zurück
- `fit_models(project_id, model_ids[])` — mehrere Modelle fitten (`fitModelsToData`)
- `run_postfit_analysis(project_id, model_id, method, config)` — MC/CV/Reduction/WGS/MOC
- `calculate_bc50(project_id, model_id)` — BC50 aus den Fit-Parametern

**Mutate / FileWrite (mit Freigabe):**
- `create_model(project_id, model_id, initial_params?)` — `CreateModel` + zu Projekt
- `set_model_parameter(project_id, model_id, name, value)` — Startwert vor dem Fit
- `load_project(path)` — Projekt laden
- `save_project(project_id, path?)` — Persistierung
- `export_results(project_id, path, format)` — JSON/CSV-Export

## Risks

- **Erste Netzabhängigkeit.** `Qt6::Network` ist Teil von qtbase (kein zusätzliches
  CI-Modul). HTTPS braucht Qts TLS-Plugin — für AppImage/Windows/macOS prüfen, dass
  das OpenSSL-Backend mitgepackt wird (Qurcuma nennt das als wahrscheinlichsten
  Ausrutscher).
- **Threading.** Fits sind rechenintensiv und blockieren — der Agent-Loop muss auf
  einem Worker-Thread laufen, GUI-Aufrufe über den Dispatcher marshallt. ProjectManager
  ist thread-sicher, aber die GUI ist es nicht.
- **Kontextkosten.** Der Werkzeugkatalog geht bei *jedem* Request mit. Nach jeder
  Erweiterung `catalogueBytes()` prüfen; `available`-Prädikat nutzen; Ergebnisse
  kappen (`truncated`, `maxToolResultChars`).
- **Prompt-Injektion.** Projektnamen, Modellnamen und Logzeilen landen als
  Werkzeugergebnisse im Modellkontext. Sie sind Daten, keine Anweisungen. Die
  Absicherung ist die Freigabepolitik.
- **Lokale Modellqualität.** Ob ein *lokales* Modell Werkzeuge zuverlässig aufruft,
  hängt am Modell, nicht am Endpunkt (Qurcuma hat das gegen `glm-5.3-flash:cloud`
  gemessen, nicht gegen ein lokales). Testmodell: `ornith-1.5:9b`.
- **Eigen-ABI.** Qurcuma dokumentiert ein `double free`, wenn `-march`-Flags eines
  Ziels von curcumas abweichen. SupraFit hat `USE_AVX2` (`CMakeLists.txt:88`) —
  jedes neue Ziel muss den Flag-Block spiegeln.

## Verification

**Automatisiert (kein Netz in ctest):**
- `test_toolregistry` — Schema-Wohlgeformtheit, Pflichtfelder, Typfehler, `enum`,
  unbekannte Schlüssel, Doppel-Registrierung
- `test_tooldispatcher` — Thread-Marshalling, Timeout, Audit
- `test_llmclient` — gegen `QTcpServer`-Stub: Anfrageform, `Authorization`,
  `tool_calls`, HTTP-Fehler, kaputtes JSON, SSE-Zerlegung
- `test_llmsession` — gegen Stub mit vorbereiter Antwort-Folge: schlichte Antwort,
  Tool-Aufruf mit Rückgabe, verweigerte Freigabe, Iterationsgrenze, Transportfehler
- Bestehende Suites bleiben grün

**Operator-Prüfpunkte:**
1. Nach Phase 3: `suprafit_cli --agent` noch nicht — erst Phase 6.
2. Nach Phase 5: „Lade Projekt X, fitte alle Modelle, vergleiche per AIC" — Read-Tools
   ohne Rückfrage, `fit_model` mit Freigabe.
3. Nach Phase 6: Leitszenario oben gegen lokales Ollama-Modell (`ornith-1.5:9b`).
4. Audit-Spur ansehen: was hat das Modell aufgerufen, was wurde freigegeben.
5. Modell nicht erreichbar → Klartext-Fehler, kein Hängen.
6. `cancel()` während eines laufenden Fits → sauberer Abbruch.

**Messungen (erst gegen echtes Ollama, nicht in ctest):**
- Katalog-Größe in Bytes nach jeder Tool-Erweiterung (`catalogueBytes()`).
- Runden-Anzahl für das Leitszenario pro Modell.
- Passt das Modell die AIC-Ranking korrekt an den Operator weiter?
