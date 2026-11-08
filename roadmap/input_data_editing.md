<!--
Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
Claude Generated - work package / roadmap document (no production code changed)
-->

# Work Package: Eingabedaten editieren (Traces: UV-Vis, NMR, Thermogramm)

**Status:** ADD (Plan nach Diskussion überarbeitet) · **Branch:** `feature/input-data-editing` · **Owner:** Conrad Hübler

## Ziel

Thermogramm-, Spektren- und (neuer) NMR-Import laufen über eine gemeinsame, GUI-freie Trace-Pipeline im Core;
die GUI rendert nur. Stil wie bei `ItcProcessor`: der Core besitzt die Wissenschaft und die Persistenz.
NMR bekommt als Basisvariante Peak-Picking + Signalverfolgung (δ-Tabelle), als Advanced-Variante einen
Gauss-Lorentz-Fit des ganzen Signals (Lage, Breite, Fläche, Mischung).

## Entscheidungen (Diskussion 2026-11-08)

- Das Thermogramm zieht **zuletzt** um (Handler frisch refaktoriert, GUI-Abnahme steht aus).
- NMR-Format zum Start: **ASCII/CSV** (TopSpin-/MestReNova-Export). Bruker `1r`, JEOL, FID→FFT: TODO.
- Lineshape: **Pseudo-Voigt** (lineare Gauss-Lorentz-Mischung). Echtes Voigt: TODO.
- Globaler Fit (Spektrum direkt am Bindungsmodell): **späte Phase**.
- QBit und libpeakpick-`GLFit` sind **Ideenquelle**. Im Zweifel neu und sauberer im SupraFit-Stil schreiben
  (Eigen-Functor, keine Debug-`cout`, Tests), nicht portieren.
- Komplizierte Teile (manuelle Zuordnungs-Editoren, Streifenverarbeitung, Binärformate) in spätere Phasen.

- **Ort des Editierens:** ein `TraceEditorWidget`, zwei Gastgeber. Import bleibt im Dialog; das nachträgliche Editieren wird nach WP3 eine Seite „Rohdaten“ in der Datenansicht („Edit Data“ schaltet dorthin um). Bis dahin bleibt der Dialog.
- **Bestehende Fits beim nachträglichen Editieren:** als **veraltet markieren, nicht löschen**; der Nutzer entscheidet über erneutes Fitten.
- **Arbeitsweise:** Jedes Work Package wird von einem **Opus-Agent** implementiert; die koordinierende Sitzung briefet, prüft Diff und Build und committet.

## Architektur

| Schicht | Inhalt |
|---|---|
| `XYTrace` / `TraceSet` | Rohkurve (x, y, Achse, Einheit, Name); geordnete Sammlung = Titrationsschritte |
| `TraceOps` | GUI-freie, JSON-serialisierbare Operationen: Bereich, Glättung, Normierung, Baseline, Peak-Picking (dünn über libpeakpick) |
| Profile | `ItcProcessor` (bleibt), `UvVisProfile`, `NmrProfile` → jeweils eine `DataTable` |
| `LineshapeFit` | modellfreier Pseudo-Voigt-Fit (Eigen-LevMar-Functor, Parameter-Sperren frei/fix/Grenze), Basis für NMR-Advanced |
| `TraceEditorWidget` | gemeinsames GUI-Gerüst: Chart, Bereichsauswahl, Operationsliste; Profile hängen Zusatzpanels ein |
| Persistenz | ein Block `raw.<profil>` mit Operationsliste; Rohdaten nur mit `StoreRawData` |

## Work Packages (je ein baubarer Commit)

- **WP0 Aufräumen:** Spektren-Bugs (letztes Spektrum fehlt, `XEnd`←`XStart`, hartes `/1000`, stilles Verwerfen kurzer Spektren, `VarCovarSelect` `end()`-Zugriff) und die von uns genutzten libpeakpick-Fehler (`X(-1)`, `baseline.h:484`, `nxlinregress.h:56`); Tests zuerst. Vorher jede Fundstelle im Code verifizieren.
- **WP1 Core:** `XYTrace`, `TraceSet`, `TraceOps` + Tests; `SpectraHandler` zieht darauf um (Verhalten unverändert, Pin-Tests).
- **WP2 GUI-Gerüst:** `TraceEditorWidget`; Spektren-Dialog zuerst darauf; `ImportData`-Zweig fürs Wiederöffnen (Spectrum).
- **WP3 NMR Basis:** ASCII-Import (ppm-Achse, Referenz-Shift), Peak-Picking, Signalverfolgung (automatisch nächster Peak zum Vorgänger, manuelle Korrektur), δ-Tabelle für die bestehenden NMR-Modelle; Test mit simuliertem Spektrensatz.
  - 🤖 ⚙️ ADDED 2026-11-08: `NmrHandler`, `TraceIO`, `ShiftAxisOp`, Dialog `NmrImport`; Ablage als Spectrum-Projekt mit `"Profile": "nmr"` (kein neuer `DataType`). Auswahlregel: Fenster um letzte Position und lineare Vorhersage, Kosten = Abstand zur Vorhersage/Fenster + |ln Höhenänderung|. Offen: Klick-Abnahme, echte Messdaten, CLI-Import (TODO.md).
- **WP4 NMR Advanced:** `LineshapeFit` pro Spektrum (Startwerte aus Peak-Picking, Breite pro Signal frei oder über die Titration geteilt), Ausgabe δ/Breite/Fläche; Test an synthetischen Linien (Genauigkeit über Streuung der Breiten/Überlappung, schlechtester Fall).
  - 🤖 ⚙️ ADDED 2026-11-08: `src/core/trace/lineshapefit` (Pseudo-Voigt, eigener gedämpfter Gauss-Newton mit analytischer Jacobi, Grenzen per Projektion/Active Set, Kovarianz, Serienfit mit geteilter Breite/η, Normalgleichungen blockweise); `NmrHandler` Betriebsart „Lineshape fit“ (Gruppen überlappender Signale, Nachbarlinien, Konflikte werden durch den Fit getrennt, Status „fit failed“, Einstellungen in `"LineshapeFit"`, Ergebnisse beim Laden neu berechnet); Dialog-Schalter, Detailspalten, Fitkurve + Residuum. Offen: Verfolgungsfehler < 0,75 FWHM, Gewichtung, Klick-Abnahme (TODO.md).
- **WP5 Thermogramm umziehen:** `ThermogramHandler` auf `TraceOps`; `thermogramwidget`/`setUi` aufteilen; erst nach der GUI-Abnahme in `TODO.md`.
  - 🤖 ⚙️ ADDED 2026-11-08 (verhaltenserhaltend): Rohkurve als `XYTrace`, libpeakpick bekommt daraus eine `PeakPick::spectrum`; Chart-Serien werden berechnet statt gehalten; `StoreRawData` an einer Stelle; `"Thermogram"` verlustfrei (alte 6-Stellen-Blöcke laden wie bisher); `setUi()` in sechs Teile; toter Code raus; Konstruktor-Absturz des Dialogs behoben (Skalierung wurde vor Tabelle/Serien gesetzt). Zahlen: `test_thermogram_characterization` (Golden der alten Implementierung, 10 Fälle, rel. 1e-9), Pin-Test unverändert; Smoke-Test `test_thermogram_dialog`.
  - Bewusst nicht: Integration/Baseline/Peak-Grenzen auf `TraceOps` (ändert Zahlen, Track B); Chart auf `TraceEditorWidget` (der Editor wählt immer horizontal und baut Serien neu, das Thermogramm zoomt per Rechteck, wählt nur im Peak-Modus, hält Serien, braucht Tasten/Doppelklick/Skalierung – nötig wären ein Auswahlmodus „aus“, stabile Serien-Hooks und Signale für Doppelklick/Tasten im Editor). GUI-Klickabnahme steht weiter aus.
- **WP6 Doku:** `docs/ASSISTANT_HELP.md` (je WP, wo sichtbar), `AIChangelog.md`, CLAUDE.md der betroffenen Verzeichnisse.

## Später (TODO)

- Bruker `1r`, JEOL `jdf`, FID→FFT (Phasenkorrektur) als Loader.
- Echtes Voigt als Lineshape-Option.
- Globaler Fit: Spektrum direkt am Bindungsmodell (Spezies-Anteile bestimmen die Lage, Fast-Exchange, optional Linienverbreiterung); gestapelter Residuenvektor mit geteilten Parametern, evtl. VarPro für die Amplituden.
- Manuelle Zuordnungs-Editoren für verfolgte Signale.
