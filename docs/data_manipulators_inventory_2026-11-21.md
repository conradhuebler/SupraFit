<!--
Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
Claude Generated - inventory of the data manipulators of SupraFit (read-only survey of the working tree of feature/input-data-editing)
-->

# Inventar der Daten-Manipulatoren in SupraFit (2026-11-21)

Zweck: Grundlage für das Rezeptmodell (typisierter Datenfluss, siehe [roadmap/recipes_and_agent_tools.md](../roadmap/recipes_and_agent_tools.md)
und [roadmap/data_pipeline_work_packages.md](../roadmap/data_pipeline_work_packages.md)), damit kein Schritt vergessen wird.
Nur gelesen, nichts ausgeführt. **[B]** = im Code beobachtet, **[V]** = Vermutung. Typen: K Kurve, KS Kurvenserie, P Peakliste,
T Tabelle, M Modell, E Einstellung, Pr Projekt. Pfade relativ zur Repository-Wurzel.

## 1 ITC / Thermogramm
**Importfilter:** `.itc` (`toolset_io.cpp:215` LoadITCFile; K+P+Volumina+Systemparameter; T+273,15, Zellvolumen ×1000; das Ausgabeargument
`offset` verwirft jeder Aufrufer; fehlt die Datei, wird still `lastdir/<Name>` genommen; Fehler als `throw int`); xy/`.dat` (`:181`
LoadXYFile, Zeilen mit ≠ 2 Feldern still übersprungen); `.dh` (`filehandler.cpp:258` ReaddH); CLI `ReadITC` (`filehandler.cpp:288`);
Python `read_itc` (`python/bindings/module.cpp:215`: **eigene** Peak-Regel Dauer = (Ende−Anfang)/(n+1), `@`-Peaks verworfen, kann von GUI/CLI abweichen [V]).

**Fester Kern-Algorithmus** (`ItcProcessor::process()` itcprocessor.cpp:100-115; Parameter in `raw.experiment.fit` / `raw.dilution.fit`,
`getThermogramParameter` thermogramhandler.cpp:514-549: ConstantOffset, ThermogramBegin/End, PeakDuration, PeakCount, CalibrationStart/Heat,
ScalingFactor, AverageDirection, IntegrationScheme Custom|Zero|Threshold, PeakRuleList, PeakIntegrationRange, IntegrationRangeThreshold,
Iterations; InjectVolume auf Ebene `raw`). Reihenfolge im Code:
1. `Initialise()` Z.88: Parameter laden, Standardgrenzen, Regeln aus PeakCount/PeakDuration oder `CompressRules()` (Z.635) aus der `.itc`-Peakliste → `FitBaseLine()`.
2. `UpdatePeaks()` Z.270: Regeln → Fenster, `FindMaximum/Minimum`, Basislinien-Startwert (Summe darüber toter Code Z.275-311), `CalibrateSystem()`.
3. `AdjustIntegrationRange()` Z.551: Bereiche je Schema, `FitBaseLine`, bei Threshold bis `Iterations`× `ResizeIntegrationRange()` Z.603 (libpeakpick).
4. `IntegrateThermogram()` Z.315: `CalibrateSystem()` Z.486 (Kalibrierpeak = letzte `CalibrationStart` Punkte, Verhältnis Heat/Integral), Mittelung (Z.331-347, zwei Integrationen `int_end` und `int_end−1`), `FitBaseLine()` Z.410 (PeakWise, Polynom 2), `ApplyThermogramIntegration()` Z.376 (`IntegrateNumerical`), `ApplyScaling()` Z.362: `q = raw·ratio·ScalingFactor − ConstantOffset`.
5. Dilution: dieselben Schritte am zweiten Handler (nur bei `m_use_dilution`).
6. `recomputeNetHeat()` Z.117: exp − dil je Index. 7. `resultTable()` Z.133: (Volumen, Nettowärme).
Weitere: `setUniformInjectionVolume` Z.59, `padInjectionVolumes` Z.64, `setInjectionVolume` Z.50, `setScalingFactor` Z.78, `setDilutionEnabled` Z.70, `ConvertRules` Z.625.
Tests: ThermogramCharacterization (Golden, relativ 1e-9), ItcThermogram (`testAbsoluteIntegralsPinned`), ThermogramDialog.
**Auffälligkeiten [B]:** Offset-Spinner `m_const_offset` (thermogramwidget.cpp:264-277) ruft nie `setConstantOffset` (ConstantOffset nur aus JSON); Offset wird **nach** der Skalierung abgezogen (Z.369, GUI-Hinweis sagt „raw unit“); `CalibrationStart` ist im Handler eine Punktzahl (Z.492), im Widget eine Zeit (`:969`), in `Initialise` als Zeit verrechnet (Z.112) [V Einheitenmischung]; `AverageDirection` Standard true, GUI schaltet nur bei „Zero“ frei; `LegacyLoad` prüft `peak_time`, liest `end_time` (Z.250); `PeakRuleList`/`PeakIntegrationRange` mit 6 Stellen; GUI-Pfad `setExperimentFile` ruft nur `Initialise`+`IntegrateThermogram` (thermogram.cpp:418-419), nicht `UpdatePeaks`/`AdjustIntegrationRange` wie `process()` [V GUI und CLI können verschiedene Zahlen liefern]; `CutBefore`/`Overshot` nicht gespeichert; `m_gradient` ungenutzt; md5 als Binärtext (thermogram.cpp:717). Tian-Korrektur: nicht gefunden (nur kinetisches Modell `TIANModel`).
**GUI-Aktionen:** Peak-Regeln Add/Remove/Clear/Load/Write/Convert (thermogramwidget.cpp:159-240, 1076, 1116); Start/Ende/Inject-Zeit per Doppelklick (:282-330, 945-1000, setzt auch `qApp`-Property `calibration_start`); Integrationsbereich je Peak per Rechteck (:925); Schema/Iterationen/Schwelle/Before/Overshot (:387-449, 893); Skalierung cal→J (:417-430); Kalibrierwärme (:365); Systemparameter (thermogram.cpp:114-142); Uniform volume/Volumen je Zelle/Import Row (:154-165, 680, 857); Dilution laden/leeren (:583, 662); Export `.dh`/`.dat` (:810, T−273, V/1000, 6 Stellen); QSettings `thermogram/peaks_start=60`, `peaks_time=150`, `iterations=15` (Startwerte, hängen vom Rechner ab).

## 2 UV/Vis (`SpectraHandler`, spectrahandler.cpp; `SpectraWidget`)
Quellen: Tabellendatei (:57, `TableFile`), Einzeldatei (:77, Loader nach Namensteil „csv“/„absorb“), Verzeichnis (:91, natürliche Sortierung `QCollator`, gebietsschemaabhängig). Schritte: Spektrum entfernen (:236, kein „ausschließen“); TraceOps (:246); gemeinsamer Bereich `ParseData` (:148, wird bei jedem `add` zurückgesetzt); Resampling auf Spektrum 1 (:200-220, traceset.cpp:140; unmöglich → Spektrum still weggelassen); x-Werte setzen (spectrawidget.cpp:95-150, `XValues` 6 Stellen); `VarCovarSelect` (:421, Kovarianz :393, `processEvents` in der Schleife; nur das Ergebnis `XValues` gespeichert, nicht Anzahl/Cluster/Average); Cluster (:483-507); `CompileSimpleTable` (:119, nächster Punkt ohne Toleranz, `IntensityDivisor` 1000 ohne GUI/CLI-Zugang); Konzentrationen anhängen (spectrawidget.cpp:417); `PCA` (:270, toter Code); Export (spectrawidget.cpp:468).

## 3 NMR (`NmrHandler`, nmrhandler.cpp; `NmrImport`)
Reihenfolge im Code (nmrhandler.h:162-256, `processedEntry` :510): Datei (JEOL: FID → `FidProcessing`) → `ShiftAxis(ReferenceOffset)` → `TraceOps` → Peaks (letzter PickPeaks-Schritt **oder** fester Rückfall 0,05 relativ (:536) **oder** Fensterschwelle (:619)) → `track()` (:1402) → optional `fitLineshapes` (:1500) → `shiftTable`/`shiftErrorTable`/`importTable` (:2082/2110/2135).
Bausteine: ASCII/CSV/totxt (`traceio.cpp:220`), JEOL (`jeolreader.cpp:400`, Filter-Gruppenlaufzeit :189), FID→Spektrum (`fidprocessing.cpp:589`, `fromJeol` :620, `complexSpectrum` :357, `phase` :404, `autoPhase` ACME :442; Standard LB 0,3, ZeroFill 2, AutoPhase true), Referenz-Offset (:489, verschiebt Signale/Handwerte mit), Referenzspektrum (:678), Signale/Multipletts (:683-766, :1031 `addMultiplet`, :1070 `pickMultiplets`, Zentrum :872), Schwelle (:611/:619, 6σ/10σ), Verfolgung (:1402/:1179; `MinHeight` 0,3 ohne GUI/CLI/Agent-Zugang), Handwerte `Manual` (:2002/2011), Ausschlüsse `Excluded` (:2030), Lineshape-Fit (:1500, `lineshapefit.cpp`), Tabelle/Join (:2082/2135).
**Auffälligkeiten [B]:** Handwerte/Ausschlüsse hängen an der Position (:2405-2418, bei fehlendem Spektrum verworfen); Rückfall-Pick-Parameter fest verdrahtet; `Data` bei FID-Eintrag beim Laden übersprungen (:2320-2328); `StoreAbsolutePath` nicht beachtet (:2208); Cache nicht thread-sicher; zwei Achsverschiebungen (ReferenceOffset und ein ShiftAxis-Schritt) möglich [V].

## 4 Trace-Schritte (`traceops.{h,cpp}`, GUI `traceeditorwidget.cpp:242-253,471`)
`Range`, `Smooth` (Savitzky-Golay, Window 5, Order 2), `Normalise` (Max/Area/MinMax, Referenzfenster; vor `Blank` zu stellen, wenn die Referenz ausgeblendet wird), `Baseline` (Polynom Grad 1, Excluded[]), `ShiftAxis`, `Blank`, `PickPeaks` (K→K **und** Nebenausgabe P; `TraceOpList::apply` reicht nur das Ergebnis des **letzten** PickPeaks heraus, cpp:864-890: im Rezept Typregel K→P). Hilfen: `trapezoid`, `noiseLevel`, `XYTrace::range/interpolate/nearestIndex`, `TraceSet::commonRange/resampleToCommonAxis`. Persistenz: `TraceOps` im raw-Block (UV/Vis, NMR); im ITC-Pfad nicht vorhanden.

## 5 Tabellen-Manipulation der GUI und des Imports
- **„Show Raw Data“ + Ausdruck im Spaltenkopf** (datawidget.cpp:256-300 → `dataclass.cpp:900 ApplyCalculationModel`, QJSEngine): JS-Ausdruck in X1..Xn/Y1..Ym; **kein Test**. Mängel: Xj+1 wird erst beim Bearbeiten von Spalte j gesetzt (veraltete Werte), Y sieht kein X, NaN/Fehler → stiller Rohwert, kein Neuberechnen beim Laden, kein Zeitlimit, `||` zerbricht den Kopf.
- **Shift begin data** (datawidget.cpp:331): nur bei einer unabhängigen Spalte, schreibt „(X1 − x0)“ als Kopftext (6 Stellen).
- **Begin/End Data** (datawidget.cpp:302-329, `UpdateCheckedState` dataclass.cpp:936): löscht alle Marken und setzt nur den Bereich neu (einzeln ausgeblendete Punkte gehen verloren: reihenfolgeabhängig); **Punkt aus-/einschließen** (datawidget.cpp:394 → `DataTable::CheckRow` datatable.cpp:284).
- Hide Datapoints, Plot X Values: nur Ansicht. Regression-Dialog (regressionanalysisdialog.cpp:68-102): Analyse.
- **Split Data Set** (modeldataholder.cpp:204, 1768): kopiert den ganzen `raw`-Block in jedes Teilprojekt (NMR/UV/Vis passt nicht zur einen Spalte), kein Test.
- Tabelle editieren (datatable.cpp:253; datawidget.cpp:389); **DropTable** Ctrl+V/B/Y (DropTable.h:60-115; Ctrl+B hängt Spalten an, Ctrl+Y stellt Spalten voran; `deleteRow/deleteColumn` deklariert, nie implementiert).
- **Import-Parser** (filehandler.cpp:106 `ReadSeperated`, :152 `ReadGeneric`, :193 `ConvertTable`): Trenner (Leer/Tab; `,` nur bei `.csv`), `#` Kopf/Kommentar, Dezimalkomma (`,`→`.` immer: Konflikt bei `1,5;2,3` [V]); `# unabhängige Spalten`/Spalten weglassen (importdata.cpp:130, 480 → `FromImportTable` dataclass.cpp:481; Vorgabe aus `qApp`-Property `lastSize`); ConcentrationEditor (concentrationeditor.cpp:105-146); Export-Tabelle (importdata.cpp:441: `ReadWrite` ohne `Truncate` :451, Reste einer längeren Altdatei bleiben stehen).
- **Einheiten: keine Tabellen-Einheitenumrechnung gefunden** (gesucht in importdata, filehandler, datatable, units.cpp; dort nur die Energieeinheit).

## 6 Datenerzeugung und Simulation
„Simulate Data“ (importdata.cpp:136, 212-344 → `datagenerator.cpp:53 Evaluate`, QJSEngine, X = Zeilennummer + 1, Gleichungen durch `|`); Store/Load Recipe (importdata.cpp:234/243; **Fehler**: sucht `main` in `m_project` statt in `data`, :250-257); Zufallsparameter (datagenerator.cpp:82, 114-178, 224; Seed `currentMSecs` bei 0); modellbasierte Daten (:297, 366-372, :233/259); `GenerateDataDialog` (generatedatadialog.cpp:105; Slot nirgends verbunden, vermutlich unerreichbar); Export Simulated (exportsimulationwidget.cpp:69-211 → `PrepareMC` datatable.cpp:637/654, `PrepareBootStrap` :678; Zufall aus der Uhrzeit); CLI `DataFactory` (data_factory.cpp:80,165,231; Rauschen 0,05 **absolut**, Kommentar sagt 5 %, Seed 42, überschreibt auch die Rohtabelle); CLI `GenerateDataWithDataGenerator` (suprafit_cli.cpp:1228); Python `generate_independent/dependent` (module.cpp:345/376, mt19937_64, Seed 0). **Drei bis vier Rausch-Generatoren mit verschiedenen RNGs und Seeds.**

## 7 Statistik und Modell-Seite (kein Rezeptschritt, aber Datenwirkung)
Monte Carlo (montecarlostatistics.cpp:247-400; Seed `RandomSeed`, sonst Uhrzeit), Cross-Validation/Jackknife/Reduction (resampleanalyse.cpp:70, 419, `DisableRow`), WGS/GlobalSearch/Landscape (Parameter, nicht Daten), aktive Serien (AbstractModel.h:442-461; Agent `set_series_included`), Optionen mit Datenwirkung (AbstractItcModel.cpp:155 `Dilution`, Reservoir :133/233-290; AbstractTitrationModel.cpp:87/94 PlotMode, HostGuestAssignment; fl_any Innerfilter/Quenching; InputQuantity, „Fix Host Signal“, „Silent Host/Guest“), **Reservoir-Auto-Wahl beim Laden** (AbstractItcModel.cpp:365-385: wählt still nach SSE < 1e-6), ScriptModel x-Ausgabe (scriptmodel.cpp:667-692, nur Ansicht; `X1` trifft auch `X10`), Meta-Modelle (nicht im Detail gelesen). **Keine Gewichtung gefunden.** **„leave out first n points“ ist ein toter Pfad:** `setStartPoint` wird nie gerufen, CLI `StartPoint` (suprafit_cli.cpp:350/653) kommt nie an (filehandler.cpp:203).

## 8 Datenersatz / Edit Data
`EditDataController::RunImportDialog` (editdatacontroller.cpp:77), `apply` :193, `undo` :241; `DataClass::checkReplacement` (dataclass.cpp:402); `ProjectManager::replaceProjectData` (projectmanager.cpp:1612), `importReplacementData` :1589, `undoLastDataReplacement` :1757; Tabellenprojekte ohne Dialog (`usesDialog` :66). Tests test_data_replace, test_edit_data.

## 9 Skript-Engines
**QJSEngine** (JavaScript, voller Umfang, **ohne Zeitlimit**): Kopfausdrücke (dataclass.cpp:902), Datengenerator (datagenerator.cpp:62/91), generatedatadialog, scriptmodel.cpp:676. **ExprTk**: nur Skriptmodelle (exprtkinterpreter.cpp), nicht Daten. ChaiScript/Python/Duktape: optionale Modell-Backends ohne Datenwirkung. Einstellung `ScriptTimeout` 500 ms gilt für Skriptmodelle [V]. Datenmanipulation per Skript gibt es heute nur über QJS.

## 10 CLI, Python, octave, scripts
CLI (client/main.cpp:812-905): `-p/-u/-t`, `-s` Split, `-j` Join, `--exclude-raw-data` (nur ML-Export); setzt `StoreRawData=true`, `InitialiseRandom=true` (:962-965); Steuer-JSON (suprafit_cli.cpp:102-231): `IndependentRows`, `StartPoint` (wirkungslos), `GenerateIndependent`/`NoisyIndependent`/`Dependent`/`NoisyDependent`/`GenerateData`, `Tasks: DataOnly`; Tabellen-Eingabe über FileHandler mit festem `m_rows = 2`; `cleanProjectForML` (:2153); ML-Pipeline (ml_cli_main.cpp, ml_pipeline_manager.cpp:465). Python: `_data.py` (from_arrays/from_file/from_equations), pybind `module.cpp`. octave/: nur Referenzfunktionen. scripts/: nur Build/Deploy.

## 11 Agenten-Werkzeuge (heute)
`set_rows_included` (tools_inspect.cpp:620, Mutate, Affinity Any; projektweit über `ReReadCheckedState`), `set_series_included` (:561), `set_model_parameter` (:489), `add_model`/`define_script_model`/`add_reaction_model`, `fit_model(s)`, `run_postfit_analysis`. **Kein** Werkzeug importiert Daten, bearbeitet Rohdaten/Tabellenwerte oder ruft `replaceProjectData`. **[B] `ReReadCheckedState` (dataclass.cpp:954) setzt `end_data` = letzter markierter Index (inklusiv), `UpdateCheckedState` nutzt `i < DataEnd` (exklusiv); nach Speichern und Laden (`setDataEnd(end_data)` :359) fällt [V] die letzte eingeschlossene Zeile heraus.**

## 12-15 ML, Einstellungen, Formate, Darstellung
ML-Vorbereitung (mlfeatureextractor.cpp:198-538): Merkmale aus Fit-/Statistikergebnissen, nicht aus Rohkurven. **Einstellungen mit Datenwirkung** (settingsregistry.cpp): `StoreRawData`, `StoreFileName`, `StoreAbsolutePath` (im NMR-Pfad nicht beachtet), `StoreFileHash` (versteckt; md5-Fehler), `FindFileRecursive` (versteckt; `FindFile` ignoriert den Parameter, toolset_io.cpp:63), `lastdir`, `auto_thermo_dialog`, `energy_unit_kcal` (nur Ausgabe), `InitialiseRandom`, `lastSize`, `calibration_start/heat`, QSettings `thermogram/*`, `nmrimport/*`, `traceeditor/*`; **Rundung**: Tabellen, `XValues`, `InjectVolume`, `PeakRuleList` mit 6 Stellen (toolset.cpp:126-149), Rohkurven verlustfrei. **Formate:** Lesen `.suprafit`/`.json` (qCompress erkannt), `.jdat`, `.dat`/`.txt`, `.csv`, `.itc`, `.dh`, `.jdf`, TopSpin totxt, UV/Vis-csv/absorb; Schreiben Projekt-JSON, `.dh`/`.dat`, Tabellen-Text, ML-JSON; xlsx: nichts gefunden. **Nur Darstellung:** ChartWrapper-Ausdünnung `MaxSeriesPoints` 200, TraceEditor Stapeln/Intensität/Hüllkurve/Achsrichtung, Thermogramm scaleUp/Down, Hide Datapoints, Plot X, PlotMode, ScriptModel-Druck-x.

## Gruppierung
- **(a) als TraceOp vorhanden:** Range, Smooth, Normalise, Baseline, ShiftAxis, Blank, PickPeaks.
- **(b) im Core fest verdrahtet:** ITC-Ablauf (1-7), NMR-Ablauf, FID-Kette (Apodisation → Zerofill → FFT → Gruppenlaufzeit → Phase), UV/Vis (Bereich → Resampling → x-Auswahl → Divisor → Join), Import-Parser, DataFactory-Rauschen.
- **(c) nur GUI:** Kopfausdrücke (Core-Funktion, aber nur die GUI setzt den Kopf), Shift begin, Begin/End, Punktmarken, Split, DropTable-Concat, Spaltenauswahl, Peak-Rechteck, Regeldatei, Import Row, Uniform volume, VarCovar-Parameter, Export Simulated, Load/Store Recipe.
- **(d) Statistik/Modell,** **(e) Einstellungen,** **(f) Ansicht:** siehe oben.

## Lücken und Überschneidungen
1. Drei bis vier Rausch-Generatoren mit verschiedenen RNGs/Seeds (PrepareMC, DataGenerator, Python mt19937_64, MonteCarlo).
2. Drei Wege zu ITC-Peak-Regeln (GUI-Startwerte 60/150, `CompressRules` aus `.itc`, Python-Heuristik) und GUI-Reihenfolge ≠ `process()`.
3. Zwei Achsverschiebungen und zwei Peak-Quellen im NMR-Pfad.
4. Zwei Ausdrucks-Engines (QJS für Daten, ExprTk für Modelle); TableOps müssen QJS-Altköpfe projizieren.
5. Zeilenausschluss an vier Orten mit verschiedener Semantik (Begin/End, CheckRow, `ReReadCheckedState`, NMR `Excluded`).
6. Nur in der GUI, ohne Core-Schnittstelle: Shift begin, Split Data Set, DropTable-Concat, Spaltenauswahl, Uniform volume.
7. Parameter ohne Persistenz: CutBefore, Overshot, VarCovar-Anzahl/Cluster/Average, Pick-Parameter von `pickSignals/pickMultiplets`, Spektrenreihenfolge der Verzeichnisquelle (Locale).
8. Parameter ohne Zugang (nur JSON): `IntensityDivisor`, `MinHeight`, `ConstantOffset` (Spinner nicht verdrahtet).
9. Typregeln, die das Rezept braucht: PickPeaks (K→K + P, nur der letzte zählt); Normalise vor Blank; Resampling nur innerhalb `commonRange`; Lineshape braucht P aus Track; ITC: Kalibrierung vor Skalierung, Offset nach Skalierung; Dilution-Join setzt zwei integrierte Kurven voraus; `FromImportTable` mit 0 abhängigen Spalten = Simulation; Split muss den raw-Block anpassen.
10. Totes/Unerreichbares: `StartPoint`, `SpectraHandler::PCA`, `GenerateDataDialog`, `m_gradient`, `.itc`-`offset`, `DropTable::deleteRow/deleteColumn`.

## Wahrscheinlich übersehene oder nicht gelesene Orte
libpeakpick-Interna (baseline.h, analyse.h, mathhelper.h); `track()`/`trackDirection` (nmrhandler.cpp:1179-1460) und `fitLineshapes` (:1500-1990) im Detail; `traceeditorwidget.cpp` Formulare (:587-760); `nmrimport.cpp` jenseits der gelesenen Stellen (~1632 Zeilen); `modeldataholder.cpp` (~1959 Zeilen) außer Datenmenü/Split; `modelwidget.cpp`/`modelelement.cpp`/`signalelement.cpp`; `chartwidget.cpp`; `ProjectManager::loadProject`/`LegacyImportData`; `AbstractModel_serialize.cpp` (LegacyImportModel); `meta_model.cpp` (Datenverbund); `statisticdialog.cpp`/`scanwidget.cpp`; `analysis_manager.cpp`; `suprafit_cli.cpp` `ProcessMLPipeline` (:1899-2153); einzelne Titrations-/Kinetik-/Thermodynamik-Modelle (`EvaluateOptions`); `concentrationsolver`/`speciationengine`-Eingaben; `systemparameterwidget.cpp`, `reactioneditorwidget.cpp`, `textwidget.cpp`; `external/` (CuteChat, CuteChart) nicht durchsucht.
