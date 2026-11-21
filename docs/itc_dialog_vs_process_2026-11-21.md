<!--
Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
Claude Generated - measurement R0/F10 of roadmap/data_pipeline_work_packages.md
-->

# ITC: Dialogweg gegen `ItcProcessor::process()` (2026-11-21)

**Frage:** Der Import-Dialog (`Thermogram::setExperimentFile`, thermogram.cpp ~418) ruft `Initialise()` + `IntegrateThermogram()`,
der Core `ItcProcessor::process()` (itcprocessor.cpp ~100, ebenso die CLI über `FileHandler::ReadITC`) zusätzlich `UpdatePeaks()` und
`AdjustIntegrationRange()`. Liefern beide dieselben Nettowärmen? Nur gemessen, am Verhalten nichts geändert.

**Messung:** `test_thermogram_dialog measureDialogVersusProcess` (ctest ThermogramDialogTest, Offscreen-Dialog; SKIP ohne Proben).
Beide Wege bekommen dieselbe Peak-Regel (`ruleBlock`: eine Regel, Dauer = Spanne/21), Skalierung cal→J. Verglichen: Nettowärme je
Injektion, gleicher Index (Verschiebungen um ±1 sind deutlich schlechter, also ist Index 0 die Zuordnung). Bezug: größtes |q| von `process()`.

| Fall | Injektionen Dialog / `process()` | größte \|Δq\| / max \|q\| | ohne letzte Injektion | größte relative Abweichung (eigener Wert) |
|---|---|---|---|---|
| reaction.dat | 20 / 19 | 5,4 % (Injektion 19) | 0,63 % | 186 % (Wert nahe 0) |
| dilution.dat | 20 / 19 | 5,0 % (Injektion 19) | 0,20 % | 9,4 % |
| reaction.dat − dilution.dat | 20 / 19 | 1,5 % (Injektion 1) | 1,5 % | 122 % (Wert nahe 0) |

**Befund:** bestätigt. Die Wege unterscheiden sich in der Zahl der Injektionen (der Dialog hat eine mehr) und in den Werten: Worst Case
5,4 % des größten Signals, an der letzten gemeinsamen Injektion; innen 0,2-1,5 %. Die relative Abweichung je Wert ist bei Wärmen nahe
null groß und dort nicht aussagekräftig.

**Nicht gemessen:** `synthetic.itc` (12 Punkte): `process()` kehrt nicht zurück (`PeakPick::spectrum::XtoIndex` läuft für eine Regel in
den ersten vier Punkten endlos, Track B2 in `SESSION_HANDOFF_THERMO.md`, mit gdb bestätigt); der Dialog ruft für `.itc` kein
`UpdatePeaks` und ist nicht betroffen. Die CLI ruft für `.itc` denselben `process()` (aus dem Code gelesen, nicht ausgeführt).
Weitere ITC-Dateien liegen nicht im Repository.

**Entscheidung offen (R3):** welcher Ablauf gilt (Dialog oder `process()`), dann GUI, CLI und Python auf einen Weg bringen; die Zahlen
ändern sich für den Weg, der angepasst wird.
