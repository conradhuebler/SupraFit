<!--
Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
Claude Generated - work package document (draft for discussion, no production code changed)
-->

# Arbeitspakete: Rezept, typisierter Datenfluss, Agent und Rohdaten-Seite

**Status:** Entwurf (2026-11-21), Diskussion mit dem Betreiber offen · Grundlagen: [recipes_and_agent_tools.md](recipes_and_agent_tools.md)
(Plan, Richtungsentscheidung), [docs/data_manipulators_inventory_2026-11-21.md](../docs/data_manipulators_inventory_2026-11-21.md)
(Inventar aller Manipulatoren), [input_data_editing.md](input_data_editing.md) (bisherige Arbeit). Vault: Projekt-Notiz „SupraFit
Eingabedaten-Pipeline und Rezepte“ (Stand dort nachführen, nicht kopieren).

## Leitplanken (gelten für jedes Paket)
- Typisierter Datenfluss mit freier Reihenfolge; Schritt = registrierter, versionierter, deterministischer Operator mit Parametern; Manuelles ist expliziter Parameter, kein GUI-Zustand; Ausdrücke sind Text mit Engine-Version in einer Sandbox.
- Ein Paket = ein oder mehrere kleine baubare Commits; Core zuerst, GUI rendert; Golden-Files/Pins bleiben bitgleich, wenn nichts anderes begründet ist; Rückwärtskompatibilität der Projektdateien (neue Schlüssel optional, `qint_version` bleibt).
- Umsetzung durch Opus-Agenten, Prüfung und Commit durch die koordinierende Sitzung; ASSISTANT_HELP im selben Change (Budget wird angehoben); Vault am Paketende nachführen.
- Libpeakpick-Track B (ändert ITC-Zahlen) bleibt getrennt und wird nicht in die Zerlegung gemischt.

## Pakete

### R0 Fehler aus dem Inventar beheben (klein, unabhängig)
Je Defekt ein Commit mit Test; Liste siehe TODO.md „Findings of the data manipulator inventory“: „Load Recipe“-Schlüssel (importdata.cpp ~250), md5 des Thermogramm-Dateiblocks, Export-Tabelle ohne `Truncate` (importdata.cpp:451), `ReReadCheckedState`/`UpdateCheckedState` Off-by-one und Verlust einzeln ausgeblendeter Punkte, Offset-Spinner des Thermogramms nicht verdrahtet, `LegacyLoad` peak_time/end_time, `FindFile` ignoriert `recursive`, GUI-`setExperimentFile` ≠ `process()`. Defekte mit Zahlenwirkung (Offset nach Skalierung, `CalibrationStart` Einheiten) erst nach Entscheidung.
**Abnahme:** je Defekt ein Test, der vorher scheitert; Pins unverändert.

### R1 Rezeptkern: Typen, Operator-Registry, Validator, Provenienz
`src/core/recipe/`: Datentypen (Kurve, Kurvenserie, Peakliste, Tabelle, Skalar/Parameter), `Operator` (Type, Version, Ein-/Ausgabetypen, Parameterschema, `validate`, `apply`, `toJson`), `Recipe` (geordnete Schritte mit `Enabled`, `Origin`), Typvalidator (Typen statt Stufen), `SourceRef` (Pfad relativ/absolut, Größe, sha256 über die dekodierten Zahlen), Provenienz-Block, Algorithmus-Versionen, kanonischer Hash/Digest, strikter Parser (unbekannte Schlüssel abgelehnt, neuere Version abgelehnt), Migration aus den heutigen flachen Schlüsseln.
**Abnahme:** Roundtrip-Tests, Alt-Projekte laden unverändert (ProjectCompat, test_nmrhandler/jeol qInfo bitgleich).

### R2 Operatorkatalog 1: vorhandene Schritte als typisierte Operatoren
Hüllen um die bestehende Logik (keine zweite Implementierung): Quellen (Import-Parser, ASCII/CSV/totxt, JEOL, `.itc`, `.dh`), FID → Kurve (Apodisation, Zerofill, FFT, Gruppenlaufzeit, Phase/Autophase), Trace-Schritte Kurve → Kurve (Range, Smooth, Normalise, Baseline, ShiftAxis, Blank) und `PickPeaks` mit **Typregel Kurve → Peakliste** (nur der letzte zählt heute: Semantik festlegen), NMR (Referenz-Offset, Signale/Multipletts, Schwelle, Verfolgung Peaks → Zellen, Lineshape-Fit, Join mit Konzentrationen), UV/Vis (gemeinsamer Bereich, Resampling, x-Auswahl VarCovar/Cluster, `CompileSimpleTable`, Divisor, Join). Handwerte/Ausschlüsse mit `SourceKey` statt Position.
**Abnahme:** je Operator ein Test gegen die bisherige Funktion (bitgleich); Zahlenpins unverändert.

### R3 ITC: „ItcClassic“ v1, danach Zerlegung
Zuerst der bestehende Algorithmus als **ein** versionierter Schritt (Parameter wie heute, Golden-File `data/samples/itc/thermogram_characterization.json`, relativ 1e-9). Danach schrittweise Zerlegung in Einzelschritte (Regeln erzeugen, Kalibrierung, Basislinie je Peak, Integrationsbereich, Integration, Skalierung/Offset, Dilution-Join, Volumen-Spalte, Tabelle), jede Zerlegung mit dem Golden-File als Gleichheitstest und ihrer expliziten Reihenfolge im Rezept. Entscheidungen vorab: Semantik des Offsets (heute nach der Skalierung), Einheit von `CalibrationStart`, GUI-Pfad vs `process()`, gemeinsame Quelle der Peak-Regeln (GUI-Startwerte, `.itc`, Python-Heuristik).
**Abnahme:** Golden-File, `testAbsoluteIntegralsPinned`, ThermogramDialog; Track B getrennt.

### R4 Tabellen-Operatoren und Ausdrucks-Engine
ExprTk härten (Kontrollstrukturen aus, Stacktiefe/Längenlimit, Laufzeitfehler melden), Operatoren `DeriveColumn`, `ScaleOffset`, `NormaliseToColumn/Row`, `SubtractReference`, `ExcludeRows/Columns` (Marken), `Rename/SetUnit`, `Smooth`, `Interpolate`, `DilutionFromVolumes`, **Einheitenumrechnung** (fehlt heute); **ein** Zeilenausschluss-Modell statt vier (Begin/End, CheckRow, `ReReadCheckedState`, NMR `Excluded`); QJS-Altköpfe als Legacy-`DeriveColumn` projizieren (`ApplyCalculationModel` hat Mängel: veraltete Variablen, Y sieht kein X, stiller Rohwert, kein Zeitlimit); „Split Data Set“ mit angepasstem raw-Block; Datenerzeugung als Operator mit gespeichertem Seed (ein RNG statt drei bis vier).
**Abnahme:** `TableOps`-Tests (Muster `TraceOps`), Differenztests QJS-Alt vs ExprTk für Standardausdrücke.

### R5 Speichern in der Projektdatei
Rezept + Provenienz im Projekt (selbsttragend, Export optional); Tabellenprojekte: `raw.SourceTable` (verlustfrei) + `raw.TableOps`; `Data` der FID-Einträge nicht mehr schreiben; `FidPacked` ab ~5 MB; `TableDigest` und Verifikation beim Laden (`LoadReport`); Entscheidung zur 6-Stellen-Rundung der Tabellen (Digest nur auf dem Text oder zusätzlich exakt); Warnschwelle für große Projekte.
**Abnahme:** Speichern/Laden bitgleich auf Digest-Ebene; alte Dateien laden; Größenmessung T5 (gemessen, nicht geschätzt).

### R6 CLI und Python
`suprafit_cli --recipe r.json --files … -o p.suprafit` (schließt den offenen NMR-CLI-Import), `--verify-recipe`; Python-Bindings nutzen denselben Runner (Python-`read_itc` ersetzt die Heuristik); ein RNG.
**Abnahme:** CLI-Tests im Stil von test_cli_core; Gleichheit GUI/CLI/Python auf denselben Rohdaten.

### R7 Agenten-Werkzeuge
Hilfe-Budget anheben; `AgentFileAccess` (erlaubte Wurzeln, `canonicalFilePath`); Werkzeuge `describe_raw_files`, `get_project_recipe`, `describe_recipe_steps`, `find_nmr_signals`, `get_spectrum_excerpt` (≤ 200 Punkte), `run_recipe`, `compare_recipes`, `describe_thermogram`, `preview_table_ops` (Read/Compute); `create_project_from_recipe`, `update_project_recipe` (über `replaceProjectData`), `set_table_preprocessing`, `set_nmr_manual_shift`, `set_rows_included` angepasst, `export_recipe` (Mutate/FileWrite); `projectDataReplaced` in der GUI verbinden; Rezeptcache je Sitzung; Agenten-Ausdrücke nur über die ExprTk-Whitelist.
**Abnahme:** ein Test je Werkzeug über die Registry, Größenlimits, Stub-Server-Ende-zu-Ende „Agent erstellt aus synthetischen JEOL-Rohdaten ein NMR-Projekt“.

### R8 RawSession und Seite statt Dialog
`RawSession` je Projekt im `ProjectManager` (Handler, Operatorliste, Schritt-Undo, Herkunft Mensch/Agent, Revisionsnummer), Übernehmen über `EditDataController` → `replaceProjectData`. S1 Session ohne GUI; S2 UV/Vis-Seite; S3 NMR-Seite; S4 ITC-Seite (nach Klickabnahme). Dialoge bleiben als Import-Assistent. Layout 1366x768 mit Dock; Schrittliste mit Spalte „von“, Agenten-Schritte bis zur Bestätigung markiert.
**Abnahme:** Offscreen-Tests; Klickabnahme durch den Betreiber vor S4.

### R9 FAIR-Paket
Exportierbares Datenpaket (z. B. RO-Crate: Rohdateien, Rezept, Ergebnistabelle, Metadaten, Prüfsummen), Metadatenfelder (Titel, Messbedingungen, Einheiten, Software-Version, Lizenz).
**Abnahme:** Paket aus einem Projekt erzeugen und das Rezept daraus erneut ausführen (Gleichheit).

### R10 Parallelisierung (TODO „Parallelisation of the data processing“)
Verfolgung pro Signal, `TraceOpList::apply` pro Spektrum, Lineshape-Fit pro Gruppe, `.jdf`-Lesen; bitgleich, ohne Reduktionen über Threads, „ein Handler, ein Thread“.

## Abhängigkeiten und Reihenfolge
R0 unabhängig (sofort). R1 vor allem anderen. R2 und R4 nach R1, parallel möglich. R3 nach R1/R2 (Monolith-Schritt braucht nur R1). R5 nach R1 (Digest, Provenienz). R6 nach R2/R5. R7 nach R2/R4/R5. R8 nach R2 und einer Klickabnahme. R9 nach R5. R10 jederzeit, aber nach R2 sinnvoll.

## Offen für die Diskussion
1. Reihenfolge und Zuschnitt (R0-R10) so? 2. R3: Entscheidungen zu Offset-Semantik und `CalibrationStart`. 3. R4: Zeilenausschluss-Modell (Marken je Zeile, Begin/End als Regel?). 4. Die 14 Entscheidungen aus recipes_and_agent_tools.md (Empfehlungen dort). 5. Vorrang: R0/R1 zuerst oder zusätzlich die vier sicheren Vorarbeiten (ExprTk-Härtung, `TableOps`-Core) parallel.
