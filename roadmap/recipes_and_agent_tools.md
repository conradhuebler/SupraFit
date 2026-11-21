<!--
Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
Claude Generated - work package / roadmap document (no production code changed)
-->

# Work Package: Reproduzierbare Rohdatenverarbeitung (Rezept) und Agenten-Werkzeuge

**Status:** ADD (Plan vom 2026-11-21, Entscheidungen offen) · **Vorlage:** [roadmap/input_data_editing.md](input_data_editing.md) · **Owner:** Conrad Hübler

## Ziel
1. Die Schritte „Rohdaten → Eingabetabelle“ (ITC, UV/Vis, NMR) werden als **versioniertes Rezept** gespeichert und sind reproduzierbar: im Projekt, als eigene Datei, per CLI und per Agent ausführbar.
2. Alle Spektren- und Thermogramm-Manipulationen stehen dem LLM-Agenten als Werkzeuge zur Verfügung, als dünne Hüllen um dieselbe headless Core-Funktion (keine zweite Logik).

## Befunde, die den Plan bestimmen (aus dem Code, 2026-11-21)
- Die gespeicherte Eingabetabelle (`DataTable::ExportTable`), UV/Vis `XValues`, ITC `InjectVolume` und `PeakRuleList`/`PeakIntegrationRange` haben nur **6 signifikante Stellen**. „Bitgleich“ gilt daher nur als „neu gerechnet, auf 6 Stellen geschrieben = gespeicherter Text“.
- `docs/ASSISTANT_HELP.md` liegt bei etwa 7976 von 8000 Token (`AgentHelpTest`, Zeichen/3,5): neue Werkzeuge in Abschnitt 8 brechen den Test, solange nichts gekürzt oder das Budget angehoben wird.
- `block["md5"] = QString(fileHashed.result())` (thermogram.cpp ~717) schreibt Binärbytes als Text statt Hex; `StoreFileHash` ist ausgeblendet und aus.
- Nicht gespeichert: Version der Algorithmen, Größe/Hash der Quelldateien, Auto-Phase-Werte je Spektrum, Referenzierungsvorgang (nur der Offset), Auto-Pick-Parameter, Herkunft der Konzentrationen, Warnungen. Pfade sind absolut, `StoreAbsolutePath` wird im NMR-Pfad missachtet; Handwerte hängen nur an der Position der Spektren.
- Keine Zufallsanteile im Trace-/NMR-/Spektren-/Thermogramm-Core; OpenMP im NMR-Pfad nur je Datei ohne Reduktion (deterministisch). ITC: OpenMP-Reduktion streut ~1e-16 (Toleranz relativ 1e-9). Über Plattformen hinweg (libm, FMA) nur innerhalb einer Toleranz; die natürliche Sortierung (`QCollator`) hängt vom Gebietsschema ab.

## Rezept
- Grundsatz: **eine Sicht auf die vorhandenen flachen Schlüssel** des `raw`-Blocks, keine zweite Kopie des Zustands. Neue Schlüssel sind optional und werden nur geschrieben, wenn sie vom Standard abweichen; `qint_version` bleibt (ältere Versionen ignorieren unbekannte Schlüssel).
- Kopf: `"RecipeVersion": 1`, `"Algorithms"` (Tracking, Threshold, Multiplet, FidPhase, Lineshape bzw. PeakPick; fehlt = 1 = heutiger Stand), `"Provenance"` (SupraFit-Version, GitCommit, UTC, Threads, Plattform, `Origin` dialog|cli|agent:<chat-id>, `Sources[]` mit Datei/relativ/Größe/sha256/Reihenfolge/Spalte, `Warnings[]`, `AppliedPhases[]`, `TableDigest`).
- NMR: heutige Schlüssel plus optional `Procedure` (Referenzierung auf Peak/ppm, Pick-Parameter, Gruppierung) und `SourceKey` an `Manual`/`Excluded`. UV/Vis: plus `XValuesExact`. ITC: plus `InjectVolumeExact`.
- Migration: Alt-Projekt = Projektion der vorhandenen Schlüssel, Algorithmen v1. Beim Laden Warnung bei älterer Version und Vergleich der neu gerechneten Tabelle mit der gespeicherten (6-Stellen-Text) im `LoadReport`.
- Datei `*.sfrecipe.json` (ohne Rohdaten und Pfade), strikter Parser (unbekannte Schlüssel abgelehnt: das Registry-Schema prüft verschachtelte Objekte nicht).
- CLI: `suprafit_cli --recipe r.json --files … [--concentrations c.dat] -o p.suprafit` (schließt den offenen Punkt „CLI import of a spectra series“) und `--verify-recipe`.

## Agenten-Werkzeuge (Antworten < 6000 Zeichen, `truncated`/`offset`)
`describe_raw_files`, `get_project_recipe`, `find_nmr_signals`, `get_spectrum_excerpt` (≤ 200 Punkte, Min/Max-Hülle), `run_recipe`, `compare_recipes`, `describe_thermogram` (alle Read/Compute) sowie `create_project_from_recipe`, `update_project_recipe` (über `ProjectManager::replaceProjectData`, also mit Veraltet-Markierung), `set_nmr_manual_shift` (Mutate, Affinität Gui) und `export_recipe` (FileWrite).
- Dateizugriff: neu `AgentFileAccess` mit erlaubten Wurzeln (Projektordner, `--agent-data-dir`, „Ordner freigeben…“ im Chat-Dock), `canonicalFilePath`, Größenlimit; Ablehnung mit klarer Meldung.
- Determinismus: `RecipeCache` je Sitzung (Mutex, LRU 2 Serien, ~94 MB je T5-Serie); der Runner legt je Aufruf einen eigenen Handler an (Regel „ein Handler, ein Thread“).
- Die GUI hört nicht auf `projectDataReplaced`: neu verbinden, damit Agenten-Änderungen sichtbar werden.

## Architektur
Core `src/core/recipe/` (`ProcessingRecipe`, `SourceRef`, `RecipeRunner::run/verify`), Handler bekommen `recipe()`/`applyRecipe()` als Projektion ihrer `toJson`/`fromJson`; Agentenschicht `src/agent/tools_rawdata.*`, `fileaccess.*`; GUI-Buttons „Export/Apply recipe…“ rufen dasselbe `applyRecipe`.

## Reihenfolge (je ein baubarer Commit)
1. sha256-Hex im Core, md5-Fehler beheben (S). 2. `SourceRef` + Provenienz (M). 3. `ProcessingRecipe` mit Parser/Migration/Hash (M). 4. NMR `recipe`/`applyRecipe` (M, Alt-Pins bitgleich). 5. UV/Vis + ITC (M). 6. `RecipeRunner` + `verify` (M). 7. CLI (S/M). 8. GUI-Buttons + ASSISTANT_HELP (M). 9. `AgentFileAccess` (S). 10. Lese-/Rechenwerkzeuge + Hilfe-Budget (M). 11. Mutationswerkzeuge + `projectDataReplaced` (M). 12. Ende-zu-Ende mit Stub-Server und synthetischen JEOL-Dateien (M). 13. Doku (S).

## Offene Entscheidungen (Empfehlung in Klammern; siehe auch TODO.md „Open decisions“)
1. Rezept als Sicht auf die flachen Schlüssel oder verschachtelter Block `Recipe` (Sicht).
2. Voll genaue Tabelle zusätzlich speichern (nur Digest in der Provenienz).
3. Bei Algorithmus-Änderung alten Pfad behalten oder nur warnen (warnen und vergleichen).
4. Hilfe-Budget: **entschieden 2026-11-21: anheben** (von 8000 auf etwa 9000 Token; Werkzeuge in Abschnitt 8 nur mit Namen).
5. Datei-Policy: Projektordner automatisch zum Lesen, alles andere explizit, Schreiben nur FileWrite.
6. Bei `StoreAbsolutePath` aus: Dateiname + sha256 als Identität (ja).
7. Auto-Phase-Werte speichern, aber nicht standardmäßig einfrieren (ja).

## Ergänzungen des Betreibers (2026-11-21)
- Das Rezept soll eine **Abfolge verschiedener Schritte** speichern können (geordnete Liste typisierter Schritte über alle Stufen: FID/Achse/TraceOps/Extraktion bis zur Tabelle), nicht nur die Parameter einer festen Pipeline.
- **Speichern in der Projektdatei** ist der Normalfall (keine verteilten Dateien); der Export als `*.sfrecipe.json` ist nur optional. Mit `StoreRawData` ist das Projekt selbsttragend (Größe: Rohdaten kompakt/komprimiert ablegen prüfen).
- Auch **„normales“ Preprocessing von Tabellen** (Spalten rechnen, skalieren, normieren, abgeleitete Spalten, Zeilen ausschließen …) soll im Rezept stehen, so wie die Skript-Engine es erlaubt oder erlauben sollte (ExprTk; beim Simulieren gibt es schon „Store/Load Recipe“ für Spaltengleichungen). Ist-Zustand und Entwurf: Plan-Ergänzung offen.
- **UX-Folge:** Bearbeiten von Spektren/Thermogrammen läuft nicht mehr im modalen Dialog, sondern als nicht-modale **Seite im Hauptfenster** (so war es in `input_data_editing.md` für „Rohdaten“ schon vorgesehen), das Agenten-Dock daneben. Mensch und Agent bearbeiten dasselbe Rezept; die heutigen Dialoge bleiben höchstens als Import-Assistent für den ersten Import. Entwurf: Plan-Ergänzung offen (Handler als projektgebundene Objekte, gleichzeitige Bearbeitung, Layout 1366x768 mit Dock, Reihenfolge).
- **Parallelisierung** des Data-Processing: siehe TODO.md.

## Nicht beurteilt
Verhalten älterer Binärdateien mit zusätzlichen Schlüsseln, GEMM-Determinismus unter OpenMP, echte Plattformunterschiede, UV/Vis-`LoadData` bei fehlenden Dateien, Wettläufe der bestehenden Mutate-Werkzeuge (Affinität Any) mit der GUI.
