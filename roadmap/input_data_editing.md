<!--
Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
Claude Generated - work package / roadmap document (no production code changed)
-->

# Work Package: Eingabedaten editieren (Traces: UV-Vis, NMR, Thermogramm)

**Status:** WP0-WP6 ADDED 🤖 ⚙️ (synthetische Daten, Offscreen-Renders; keine Klick-Abnahme, keine echten NMR-Daten) · **Branch:** `feature/input-data-editing` · **Owner:** Conrad Hübler

Messungen, Worst-Case-Werte und Befunde: [docs/input_data_editing_2026-11-08.md](../docs/input_data_editing_2026-11-08.md).

## Ziel

Thermogramm-, Spektren- und (neuer) NMR-Import laufen über eine gemeinsame, GUI-freie Trace-Pipeline im Core;
die GUI rendert nur. Stil wie bei `ItcProcessor`: der Core besitzt die Wissenschaft und die Persistenz.
NMR bekommt als Basisvariante Peak-Picking + Signalverfolgung (δ-Tabelle), als Advanced-Variante einen
Gauss-Lorentz-Fit des ganzen Signals (Lage, Breite, Fläche, Mischung).

## Entscheidungen (Diskussion 2026-11-08)

- Das Thermogramm zieht **zuletzt** um (Handler frisch refaktoriert, GUI-Abnahme steht aus). Umgesetzt wurde nur der verhaltenserhaltende Teil (WP5).
- NMR-Format zum Start: **ASCII/CSV** (TopSpin-/MestReNova-Export). Bruker `1r`, JEOL, FID→FFT: TODO.
- Lineshape: **Pseudo-Voigt** (lineare Gauss-Lorentz-Mischung). Echtes Voigt: TODO.
- Globaler Fit (Spektrum direkt am Bindungsmodell): **späte Phase**.
- QBit und libpeakpick-`GLFit` sind **Ideenquelle**. Im Zweifel neu und sauberer im SupraFit-Stil schreiben
  (Eigen-Functor, keine Debug-`cout`, Tests), nicht portieren.
- Komplizierte Teile (manuelle Zuordnungs-Editoren, Streifenverarbeitung, Binärformate) in spätere Phasen.

- **Ort des Editierens:** ein `TraceEditorWidget`, zwei Gastgeber. Import bleibt im Dialog; das nachträgliche Editieren wird nach WP3 eine Seite „Rohdaten“ in der Datenansicht („Edit Data“ schaltet dorthin um). Bis dahin bleibt der Dialog.
- **Bestehende Fits beim nachträglichen Editieren:** als **veraltet markieren, nicht löschen**; der Nutzer entscheidet über erneutes Fitten.
- **Arbeitsweise:** Jedes Work Package wird von einem **Opus-Agent** implementiert; die koordinierende Sitzung briefet, prüft Diff und Build und committet.

## Architektur (umgesetzt)

| Schicht | Inhalt |
|---|---|
| `XYTrace` / `TraceSet` | Rohkurve (x, y, Achse, Einheit, Name); geordnete Sammlung = Titrationsschritte |
| `TraceOps` | GUI-freie, JSON-serialisierbare Schritte: Bereich, Glättung, Normierung, Baseline, Achsenverschiebung, Peak-Picking (eigener Eigen-Code, nicht libpeakpick) |
| Profile | `SpectraHandler` (UV-Vis), `NmrHandler` (NMR), `ItcProcessor`/`ThermogramHandler` (ITC) → jeweils eine `DataTable` |
| `LineshapeFit` | modellfreier Pseudo-Voigt-Fit, Einzelspektrum und Serie mit geteilter Breite/η |
| `TraceEditorWidget` | Chart, Bereichsauswahl, Operationsliste; Dialoge hängen Zusatzpanels ein |
| Persistenz | Rohdaten nur mit `StoreRawData` (verlustfrei); NMR als Spectrum-Projekt mit `"Profile": "nmr"` |

## Work Packages (je ein baubarer Commit)

- **WP0 Spektren-Fehler** ✅ ADDED: letztes Spektrum, `XEnd`, `IntensityDivisor`, `VarCovarSelect`. Die libpeakpick-Fehler wurden bewusst **nicht** behoben (ändern Thermogramm-Zahlen, Track B in `TODO.md`).
- **WP1 Core** ✅ ADDED: `XYTrace`, `TraceSet`, `TraceOps`, verlustfreies JSON; `SpectraHandler` darauf umgezogen.
- **WP2 GUI-Gerüst** ✅ ADDED: `TraceEditorWidget`, Spektren-Dialog darauf. Ein `ImportData`-Zweig fürs Wiederöffnen war nicht nötig (Edit Data öffnet den Dialog direkt).
- **WP3 NMR Basis** ✅ ADDED: `NmrHandler`, `TraceIO`, Dialog `NmrImport`, Signalverfolgung mit missing/conflict/manual, Ablage mit `"Profile": "nmr"`.
- **WP4 NMR Advanced** ✅ ADDED: `LineshapeFit` und Betriebsart „Lineshape fit“. Offen: Verfolgungsfehler unter 0,75 FWHM, Gewichtung mit δ-Fehlern.
- **WP5 Thermogramm** ✅ ADDED (verhaltenserhaltend): Rohkurve als `XYTrace`, verlustfreies JSON, `setUi()` zerlegt, Konstruktor-Absturz behoben. Bewusst nicht: Integration/Baseline auf `TraceOps` (Track B) und Chart auf `TraceEditorWidget` (Editor braucht Auswahlmodus „aus“, stabile Serien-Hooks, Signale für Doppelklick/Tasten).
- **WP6 Doku** ✅ ADDED: `docs/ASSISTANT_HELP.md` Abschnitt 2, CLAUDE.md von `core`, `core/trace`, `ui`, `AIChangelog.md`, Messdokument in `docs/`.

## Nächste Schritte

- Klick-Abnahme der drei Dialoge, Thermogramm-Liste in `TODO.md`; Test mit echten TopSpin-/MestReNova-Serien.
- Rohdaten-Seite im Hauptwidget statt Dialog für das nachträgliche Editieren („Edit Data“), Fits dabei als veraltet markieren, nicht löschen (Entscheidung oben; **nicht umgesetzt**).
- Track B (libpeakpick-Mathematik), danach Thermogramm-Integration auf `TraceOps`.

## Später (TODO)

- Bruker `1r`, JEOL `jdf`, FID→FFT (Phasenkorrektur) als Loader.
- Echtes Voigt als Lineshape-Option.
- Globaler Fit: Spektrum direkt am Bindungsmodell (Spezies-Anteile bestimmen die Lage, Fast-Exchange, optional Linienverbreiterung); gestapelter Residuenvektor mit geteilten Parametern, evtl. VarPro für die Amplituden.
- Manuelle Zuordnungs-Editoren für verfolgte Signale; CLI-Import für NMR-Serien.
