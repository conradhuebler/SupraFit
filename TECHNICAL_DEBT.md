# SupraFit – Technische Schuldanalyse

**Stand:** 2026-07-02 · **Codebasis:** ~96.550 LOC, 316 `.cpp/.h` unter `src/`
**Methode:** 3 Codebase-Sweeps (Hygiene, Architektur, Code-Qualität) + Faktenabgleich
gegen den aktuellen Code (Marker-Counts, `wc -l`, `git status`, `git submodule`).
Dieses Dokument ist die **Single Source of Truth** für technische Schuld und löst die
älteren Root-MDs ab (siehe [Abgelöste Dokumente](#abgeloeste-dokumente)).

> Zweck: (1) verifiziertes Schuld-**Inventar** mit Severity, (2) **Backlog** dessen,
> was einen tieferen, eigenständigen Blick braucht, (3) **Quick Wins** zur Hygiene,
> (4) **Bearbeitungsstand** bereits umgesetzter Fixes (§0) und **Deep-Dives** (§5).

**Severity:** 🔴 Hoch (Risiko/blockiert Fortschritt) · 🟡 Mittel · 🟢 Niedrig (Hygiene)

---

## 0. Bearbeitungsstand — Branch `cleanup/cli-quick-wins` (2026-07-02)

Umgesetzt (Commits auf dem Branch, noch **nicht** auf `master` gemerged):

| Commit | Inhalt |
|---|---|
| `7f3d213d` | CLI-Cleanup: Debug-`std::cout` raus; tote Command-Pattern-Schicht (`cli_command_parser`/`dispatcher`, `main_refactored.cpp`) entfernt (B1/B5/B6/B8) |
| `52b95406` | Docs auf Command-Pattern-Entfernung synchronisiert (`client`/`tests` CLAUDE.md, `REFACTORING_CLI.md`) |
| `0995a221` | Core-Quick-Wins: tote `m_stored_models`-UUID-Map + 10 ungeschützte Debug-Traces in `dataclass.cpp` raus |
| `685438d1` | **D2**: alle 12 MIGRATION POINTs auf ProjectManager-API migriert |
| `cab3cd6a` | **Bug-Fix**: `saveProjectAsJson()` serialisiert Modelle → `-x` + Model-Tabelle funktionieren wieder |
| `c23e25c7` | **Bug-Fix**: doppelte Model-Children beim Laden (COW-geteiltes `d` + Doppel-Add) |
| `29792f97` | Legacy-Member `m_toplevel`/`m_data` vollständig entfernt |
| `f3fee6f6` | Hygiene: `.gitignore` erweitert, 72 MB Output-Artefakte + Mode-Drift-Submodule bereinigt |
| `693d7ce4` | **D6** runtergestuft (CuteChart/libpeakpick = eigene Forks, kein Risiko) |
| `f830f417` | **D3**: `AnalysisReporter` extrahiert (ModelStatistics + 7 Reporting-Funktionen) |
| `fde7efa0` | **D3**: `MlExport` extrahiert (ML-Trainingsdaten-Export) |
| `ff816e5f` | **D4** (Struct): `ModelStatistics` in `core/model_statistics.h` konsolidiert (Duplikat weg) |
| `505299d0` | **D1**: Baseline vermessen (11/26), Doku korrigiert; `test_projectmanager` Compile-Fixes |
| `928fa1d8` | **D7**: `DEBUG_ON` an Compile-Definition gebunden · **D8**: 4 abgelöste Root-MDs entfernt |
| _(uncommitted)_ | **God-Object-Split**: `core/analyse.cpp` → Compute-TU + `analyse_format.cpp` (Format-TU); Fassade `analyse.h` unverändert, verbatim-Move, Golden-`-x`-Diff identisch |

Erledigt: **D2** (voll), **D6** (entschärft), **D4-Struct**, **D7** (DEBUG_ON), **D8** (Root-MDs),
Teile von **C**, **D1** (Baseline) und **D3** (2 Klassen raus, −805 LOC). Details in §5.

---

## 1. Schuld-Inventar

### A. Repo-Hygiene 🟢
| Befund | Beleg | Aufwand |
|---|---|---|
| 38 untracked Artefakte im Tree (CLI-Outputs `*-0.json`/`*-0.suprafit`, kaputte Endungen `*.json-0suprafit`) | `git status` (44 Änderungen, 38 `??`) | S |
| Große **untracked** Datenartefakte im Root (nicht committet, aber im Arbeitsbaum) | `best_fit_comparison-0.json` ≈23 MB, `simple_results-0.json` ≈2,4 MB | S |
| Scratch-Quellen + kompiliertes Binary im Root | `debug_xor`, `debug_xor.cpp`, `test_ml_simple.cpp`, `test_training_simple.cpp` | S |
| Tool-/Test-Caches im Tree | `.cache/` ≈11 MB (clangd), `Testing/`, `test_comprehensive_results/` | S |
| Loses vendored Header statt Submodul/Unterordner | `external/exprtk.hpp` ≈1,6 MB | S |
| `.gitignore`-Lücken für all das Obige | `.gitignore` | S |

> Hinweis: `src/tests/build/` ist **korrekt gitignored** – kein Debt (frühere Notiz „committet" war falsch).

### B. Submodul-Status 🟢 (kein Datenverlust-Risiko — eigene Forks)
| Befund | Beleg | Severity |
|---|---|---|
| Lokale Edits in **eigenen Forks** (`github.com/conradhuebler/*`) — **gewollt/verwaltet**, nicht durch Upstream-Update gefährdet (Klarstellung Conrad, 2026-07-03) | `external/CuteChart` (`src/series.{h,cpp}`), `external/libpeakpick` (`mathhelper.h`) | 🟢 |
| Gitlink **ohne** `.gitmodules`-Eintrag (9 Einträge, aber 10 Gitlinks) | `external/least-squares-cpp` = `160000`-Gitlink im Index; `submodule foreach` bricht ab | 🟡 |
| ✅ Mode-/Script-Drift zurückgesetzt (Hygiene-Runde) | `external/ChaiScript`, `external/duktape`, `external/fmt` | ✅ |

### C. Stehengebliebene ProjectManager-Migration 🔴
| Befund | Beleg | Severity |
|---|---|---|
| ✅ **12 MIGRATION POINTs — erledigt** (`685438d1`): auf `getCurrentProject()`/`getProjectAsJson()` migriert; die vermuteten „fehlenden PM-APIs" waren über die vorhandene API abbildbar | `src/client/suprafit_cli.cpp` | ✅ |
| Phase 2 (`TaskController`) und Phase 3 (`DataFactory`) **existieren nicht** – CLI/GUI duplizieren Job-Execution + Data-Generation | Roadmap-Ziele, keine Dateien vorhanden | 🔴 |
| ✅ **`m_data`/`m_toplevel` entfernt** (`29792f97`); nur noch `m_data_vector` (Multi-Project-Export) übrig | `suprafit_cli.{h,cpp}` | 🟢 |
| ✅ **`ModelStatistics`-Duplikat aufgelöst** (`ff816e5f`, `core/model_statistics.h`); Analyse-**Logik** noch parallel → `AnalysisManager`-Migration unvollständig (D4) | `analysis_manager.{h,cpp}`, `analysis_reporter.cpp` | 🟡 |
| `JsonHandler::` in **20 Dateien** direkt aufgerufen trotz ProjectManager | `rg -l "JsonHandler::" src` = 20 | 🟡 |

### D. God-Objects / Struktur 🟡
| Datei | LOC | Rolle |
|---|---|---|
| `src/client/suprafit_cli.cpp` | 3.727 | größte Datei + höchste Marker-Dichte; Migrationsziel |
| `src/ui/mainwindow/suprafitgui.cpp` | 2.553 | GUI-Controller/God-Object |
| `src/core/models/AbstractModel.cpp` | 2.067 | Basis aller Modelle (~252 Methoden, 67 Includes) |
| `src/core/analyse.cpp` | 1.906 | gesamte Statistik-API (Compute + Formatierung gemischt) |
| `src/core/toolset.cpp` | 1.407 | Grab-Bag-Utilities |
| `src/core/projectmanager.cpp` | 1.133 | Migrations-Kern |
| `src/core/analysis_manager.cpp` | 1.007 | überlappt CLI-Analyse-Code |

- **`AbstractModel : public DataClass`** – jedes Modell *ist* ein voller Datencontainer (starke Basiskopplung).
- **`DataClass`** – 73 Includes, zwei parallele Model-Storage-Maps (per UUID + per Pointer), live Debt-Marker `dataclass.h:206/217/228/244/404` (`#pragma message`, „will be removed").

### E. Code-Qualität vs. CLAUDE.md-Regeln 🟡
| Befund | Zahl / Beleg |
|---|---|
| `qDebug()` überwiegend **ungeschützt** (Regel: nur in `#ifdef DEBUG_ON`) | 825 gesamt; `suprafitgui.cpp` ≈133 / 1 Guard |
| ✅ **`DEBUG_ON` jetzt an Compile-Definition gebunden** (D7) — Guards greifen bei `-DDEBUG_ON=ON` | `CMakeLists.txt` `if(DEBUG_ON) add_compile_definitions(DEBUG_ON)` |
| `std::cout` in Produktionscode (Regel: `fmt` statt `std::cout`) | 363 (client 200, core 152, capabilities 7, ui 4; tests 0) |
| Dead Files auf Disk, nicht im Build | `client/main_refactored.cpp`, `ui/guitools/chartwrapper_legacy.*`, `ui/widgets/modeldataholder.cpp` |
| Nicht in CMake registrierte Tests | `tests/test_projectmanager.cpp`, `tests/test_neural_network_training.cpp` |
| Marker gesamt | TODO 14, FIXME 9, HACK 0, „Claude Generated" 632 |

### F. Build / Test / Sicherheit 🔴/🟡
| Befund | Beleg | Severity |
|---|---|---|
| **`exprtk.hpp`-Download zur Configure-Zeit mit auskommentiertem Hash-Check** (Supply-Chain-Risiko) | `CMakeLists.txt:141-144` (`#EXPECTED_HASH …`) | 🔴 |
| **Test-Gesundheit schwach** – Migration ohne grünes Sicherheitsnetz | `src/tests/CLAUDE.md`: 9/23 (39 %), `test_dataclass` Timeout | 🔴 |
| Stale Qt5-Fragmente / kaputter `testing`-Block (referenziert nonexistente Targets) | `test/CMakeLists.txt`, `CMakeLists.txt:533-539` | 🟡 |
| GUI unterdrückt Deprecated-Warnungen (inkonsistent zum Core) | `-Wno-deprecated-declarations` in `src/ui/CMakeLists.txt` | 🟢 |
| Keine Sanitizer, kein globales `-Werror` (nur `-Werror=return-type`) | `CMakeLists.txt` | 🟢 |

### ✅ Kein Debt (Positiv)
Qt6-Deprecated-APIs praktisch eliminiert (0 `QRegExp`/`foreach`/`QLinkedList`) · Core nutzt
`QSharedPointer` idiomatisch · CTest sauber registriert (27 Tests) · keine `#if 0`-Graveyards ·
`src/tests/build/` korrekt gitignored.

---

## 2. Backlog – „tiefer zu analysieren"

Reihenfolge = empfohlene Priorität. Jeder Punkt braucht eine eigene, tiefere Analyse
(nicht mit einem mechanischen Fix erledigt).

- **D1 – Test-Sicherheitsnetz zuerst** 🔴 *(Baseline 2026-07-03 vermessen)*
  **Ist-Stand (ctest): 11/26 grün (42 %)**, nicht 9/23. Zwei Korrekturen zur alten Doku:
  ✅ `test_dataclass` läuft **durch** (30s-Timeout reproduziert nicht mehr); `NeuralNetworkTest`
  besteht standalone (12/0, ctest-Fail ist Working-Dir-Artefakt). Die 15 roten Tests sind
  überwiegend **CLI-Tests** (rufen `suprafit_cli`-Binary auf, prüfen Ausgabe) mit **veralteten
  Erwartungen** — pre-existing API/Output-Mismatch, keine Build-Fehler.
  ⬜ Die 2 unregistrierten Tests sind **rewrite-level bit-rotted** (gegen alte APIs geschrieben,
  nie kompiliert): `test_projectmanager.cpp` (kompiliert+linkt jetzt nach Fixes, aber die
  Test-Daten passen nicht mehr zu `validateProjectJson`), `test_neural_network_training.cpp`
  (ML-API-Drift). Registrierung = Test-Neuschreibung, kein Wiring.
  *Nächster Schritt:* CLI-Test-Erwartungen an das aktuelle CLI-Verhalten anpassen (pro Test).
  *Dateien:* `src/tests/`, `src/tests/CMakeLists.txt`, `src/tests/CLAUDE.md`.

- **D2 – ProjectManager-API-Lücken** ✅ **ERLEDIGT** (`685438d1`, `cab3cd6a`, `c23e25c7`, `29792f97`)
  Statt neuer Fassaden-Methoden ließen sich alle 12 MIGRATION POINTs über die **vorhandene**
  PM-API (`getCurrentProject()` für DataClass, `getProjectAsJson()` für JSON) lösen. Dabei zwei
  vorbestehende Bugs gefixt: `saveProjectAsJson()` verwarf Modelle; Modelle wurden beim Laden
  doppelt als Children eingetragen. Legacy-Member `m_toplevel`/`m_data` anschließend entfernt.

- **D3 – `suprafit_cli.cpp`-Zerlegung** 🟡 *(in Arbeit — 3654 → 2849 LOC)*
  Command-Pattern-Ballast entfernt (`7f3d213d`); zwei stateless Helfer ausgelagert:
  `AnalysisReporter` (`f830f417`) und `MlExport` (`fde7efa0`). Verbleibend: die
  member-gekoppelten Cluster `DataFactory` (GenerateData*), `TaskRunner` (Work/PerformeJobs)
  und `MlPipeline` — die brauchen State-Passing-Design (kein verbatim-Lift) und idealerweise
  erst **D1** (Test-Netz). Dabei gefunden: **ML-Export ist vorbestehend kaputt** (speichert
  Trainings-JSON via `createProjectFromJson`, das Nicht-Projekt-JSON ablehnt). Siehe §5.1.

- **D4 – `AnalysisManager`-Konsolidierung** 🟡 *(Struct erledigt, Logik offen)*
  ✅ **Struct**: `ModelStatistics` in `src/core/model_statistics.h` konsolidiert (`ff816e5f`) —
  eine Definition statt zweier identischer. ⬜ **Logik/Integration**: Kein simpler Fix —
  `AnalysisManager::extractModelStatistics` wird von außen gar nicht aufgerufen, und
  `displayAnalysisResults` **ignoriert** die `analyzeFile`-Ergebnisse und re-extrahiert via
  `AnalysisReporter`. Entwirren erfordert die Entscheidung, welcher Pfad autoritativ ist
  (halb-migrierter AnalysisManager) — braucht **D1** (Tests). *Dateien:* `analysis_manager.{h,cpp}`,
  `analysis_reporter.cpp`, `suprafit_cli.cpp` (`displayAnalysisResults`).

- **D5 – `AbstractModel : DataClass`-Vererbung** 🟡
  *Frage:* Ist „Model *ist* Datencontainer" die richtige Beziehung? Tiefe Kopplungs-/
  Ownership-Analyse (auch die zwei parallelen Model-Storage-Maps in `DataClass`).

- **D6 – Submodul-Konsistenz** 🟢 *(kein Datenverlust — eigene Forks)*
  CuteChart/libpeakpick sind Conrads eigene Forks; die Edits sind gewollt. Übrig bleibt nur
  die kleine Inkonsistenz: `external/least-squares-cpp` ist ein Gitlink **ohne**
  `.gitmodules`-Eintrag. *Datei:* `.gitmodules`.

- **D7 – Logging-/DEBUG_ON-Strategie** 🟡 *(DEBUG_ON gebunden)*
  ✅ **`DEBUG_ON` an Compile-Definition gebunden** (`if(DEBUG_ON) add_compile_definitions(DEBUG_ON)`):
  die `#ifdef DEBUG_ON`-Guards greifen jetzt bei `-DDEBUG_ON=ON` (verifiziert), Default OFF
  unverändert. ⬜ Verbleibend: zentrales Logging und `std::cout` → `fmt` (~360 Stellen).

- **D8 – Doc-Konsolidierung** 🟢 *(Root-Sprawl erledigt)*
  ✅ Die 4 abgelösten Root-MDs entfernt (Inhalt in diesem Dokument konsolidiert). ⬜ Verbleibend:
  `docs/outdated/`-Graveyard (~18 tracked Dateien) — eigener Aufräum-Commit.

---

## 3. Quick Wins (Hygiene, geringes Risiko)

Kein tiefer Blick nötig – kann als eigener Aufräum-Commit laufen:

1. `.gitignore` erweitern: `.cache/`, `Testing/`, `test_comprehensive_results/`,
   CLI-Output-Muster (`*-0.json`, `*-0.suprafit`, `*.json-0suprafit`, `*suprafit`),
   Root-Scratch (`test_*.cpp`, `debug_*`).
2. Scratch-Dateien/Binaries entfernen: `debug_xor`, `debug_xor.cpp`, `test_ml_simple.cpp`,
   `test_training_simple.cpp`.
3. Große untracked Datenartefakte aus dem Arbeitsbaum verschieben/löschen
   (`best_fit_comparison-0.json` ≈23 MB, `simple_results-0.json`, `examples/ml_pipeline/*`-Outputs, `input/*`-Outputs).
4. Mode-only-Submodule zurücksetzen (`git submodule foreach git checkout .` für
   ChaiScript/duktape/fmt – **nicht** CuteChart/libpeakpick, siehe D6).
5. `external/exprtk.hpp` in Unterordner verschieben oder als Submodul führen.

---

## <a id="abgeloeste-dokumente"></a>4. Abgelöste Dokumente

✅ **Erledigt (D8):** Die folgenden untracked Root-MDs waren durch dieses Dokument abgelöst
(Relevantes übernommen) und wurden **entfernt**:

- ~~`ARCHITECTURE_CONSOLIDATION_ROADMAP.md`~~
- ~~`MIGRATION_POINT_ANALYSIS.md`~~
- ~~`MIGRATION_POINT_SUMMARY.md`~~
- ~~`PROJECTMANAGER_ANALYSIS_INDEX.md`~~

---

## 5. Deep-Dives

Code-verifizierte Detailanalysen. Status: ✅ erledigt · ⬜ offen.

### 5.1 CLI (`src/client/`)

- ✅ **Command-Pattern war tote Fracht**: `cli_command_parser`/`cli_command_dispatcher` wurden
  kompiliert, aber nie ins gebaute `main.cpp` verdrahtet (das nutzt direkt `QCommandLineParser`);
  `main_refactored.cpp` war gar nicht im Build. Alles entfernt (`7f3d213d`).
- ✅ **Ungeschützte DEBUG-`std::cout`** in `setInFile`/`LoadFile` (Emoji-Traces auf stdout bei
  jedem Aufruf) — entfernt.
- ✅ **12 MIGRATION POINTs** (Legacy `m_toplevel`/`m_data`-Reads) → ProjectManager-API (`685438d1`);
  Member entfernt (`29792f97`).
- ✅ **`-x` / Model-Tabelle waren leer**, weil `getProjectAsJson()` keine Modelle enthielt
  (`saveProjectAsJson`-Bug) — gefixt (`cab3cd6a`).
- ⬜ **`SupraFitCli` bleibt God-Klasse** (~3,6k LOC, ~8 Rollen) → D3 (DataFactory / TaskRunner /
  AnalysisReporter / MlPipeline / MlExport).
- ⬜ **`ModelStatistics` doppelt** mit `analysis_manager.h` → D4.
- ⬜ **`m_data_vector`** noch als Legacy-State übrig (Multi-Project-Export).
- ⬜ Zweiter Entry-Point `ml_cli_main.cpp` + Stray-Test `client/test_ml_pipeline.cpp`.

### 5.2 Core (`src/core/`)

- ✅ **`saveProjectAsJson()` verwarf Modelle** (`{"data": …}` ohne `model_X`) → nun via
  `DataClass::ExportChildren()` vollständig (`cab3cd6a`).
- ✅ **Model-Duplikation beim Laden**: `AbstractModel : DataClass(project)` teilt `d` (COW,
  `dataclass.cpp:208` `d = other->d`); Ctor-`AddChildren(this)` + `addModel()` trugen das Modell
  doppelt ein → Dedup-Guard in `AddChildren` (`c23e25c7`).
- ✅ **Tote `m_stored_models`-UUID-Map** in `DataClass` entfernt (`0995a221`).
- ⬜ **ProjectManager-Ownership-Smell**: `getModel`/`getProjectModels` geben `QSharedPointer`
  mit No-op-Deleter zurück (Aliasing, dangling-Risiko) — `projectmanager.cpp:631/671`.
- ⬜ **`AnalysisManager` halb-migriert** (nur `analyzeFile` verdrahtet, GUI nutzt ihn nicht,
  doppelte `extractModelStatistics`) → D4.
- ⬜ **`AbstractModel : DataClass`**: LSP-Bruch; greift an 6 Stellen direkt in `d->`-Interna
  (`AbstractModel.cpp:1408-1410,1696-1698`); teilt `d` (COW) mit dem Eltern-DataClass. **Korrektur:**
  Modelle haben inzwischen eine **eigene** `m_model_uuid` (`AbstractModel.cpp:78/121/159`); nur die
  Projekt-UUID (`d->m_uuid`) ist geteilt (die „keine eigene Model-UUID"-Notiz war veraltet) → D5.
- ✅ **`analyse.cpp`-Split (2026)**: Compute (JSON, `Calculate*Metrics`/`Extract*`) und String/HTML-
  Formatierung (`Compare*`/`AnalyseReductionAnalysis`/`FormatStatisticsString`) in getrennte TUs
  (`analyse.cpp` 1124 + `analyse_format.cpp` 838); Fassade `analyse.h` + `namespace StatisticTool`
  unverändert, verbatim-Move (Golden-`-x`-Diff inhaltlich identisch).
  ⬜ **`toolset.cpp`** (1407): 5-Domänen-Grab-Bag. **`bc50.h`** (879): 45 `inline` + 17 `std::cout` —
  gleiche fassaden-erhaltende TU-Split-Technik anwenden.
- ⬜ **Immer-an Debug im Core**: `dataclass.cpp` teilentschärft; noch ~143 qDebug gesamt,
  152 `std::cout` (bc50.h 17, Titrationsmodelle) → D7.
- 🔴 **`InitialGuess()` konvergiert nicht (von Grund auf)**: Beim Aufbau von `test_reference_projects`
  gefunden — ein frisch per `InitialGuess()` gestartetes Fit der 4 kanonischen NMR-Datensätze
  (`data/samples/NMR titrations/projects.suprafit`) landet **nicht** im Referenz-Minimum, sondern in
  einem lokalen Minimum (SSE ~1.2 statt ~0.013; `lg K` auf Default ~2.5). Re-Fit **ab** dem
  gespeicherten Optimum bleibt dagegen exakt dort. D.h. der Solver reproduziert das Minimum, findet
  es aber aus dem Startpunkt von `InitialGuess()` nicht. Zu klären: schlechte Startwert-Heuristik vs.
  fehlende Multi-Start/Boundary-Behandlung (vgl. Instructions-Block: BFGS-Alternative, DOI
  10.1039/d4sc03354j). *Beleg:* `src/tests/test_reference_projects.cpp` (Layer-2 vs. Layer-1).

> **Verifikation dieser Session:** `suprafit_cli` baut sauber; `-l`/`-x`/`--show-post-processing`
> auf einem Model-Projekt liefern korrekte Struktur, Modelle (einfach, nicht doppelt),
> Datendimensionen und System-Parameter.
