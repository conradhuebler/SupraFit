<!--
Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
Claude Generated - work package / roadmap document
-->

# Work Package: Model-Pane Navigation, Dialog Windows, More/Less (ScriptModel first)

**Status:** PLAN, Entscheidungen des Operators eingearbeitet (nichts umgesetzt) · **Branch:** `feature/model-pane-navigation` · Vorbild: [ui_redesign.md](ui_redesign.md) WP7/WP8

## Ziel

- Jeder Bereich der Model-Pane ist über eine einzige Navigationsliste erreichbar; platzhungrige Bereiche bekommen die volle Fläche.
- Fenster (Statistik, Ergebnisse, Scan, Charts, Chat) springen nicht mehr und merken sich Ort und Größe.
- Der "..more.."-Umschalter ist ein wiederverwendbares, konsistentes Bauteil mit Persistenz.

## Entscheidungen des Operators

- Data series wird Listeneintrag **mit Pin** (angeheftet bleibt sie neben der Seite).
- Statistik (und was dazugehört) wandert als Seite in die Navigationsliste, kein eigenes Fenster mehr.
- Weitere Bereiche gehören ebenfalls in die Liste, z. B. Concentration, Charts (Inventar in WP1 vervollständigen).
- "Float-Widget" war ein Irrtum: gemeint ist das **FlowLayout** (Umbruch-Unruhe), nicht das Chat-Dock -> WP4 entfällt, neues WP-F.
- More/Less: Name und Konzept bleiben (Update + Konsistenzcheck, WP5); `advanced_ui` ist eine **getrennte** Sache (WP6), nicht ersetzen.

## Ist-Stand (Belege)

- Model-Pane = Header (`modelwidget.cpp:387-450`) + vertikaler Splitter: [SectionStack | Data series] über "Fit results" (`:546-633`).
- SectionStack (`guitools/sectionstack.h`) hält nur: Model Options, System Parameter, Model Definition (nur ScriptModel), Parameter Table (nur `*_any`), Simulated Data.
- "Data series" ist ein eigenes Pane neben der Liste (`:604-618`), kein Listeneintrag; Orientierung wechselt bei 900 px (`:935`) und setzt dabei die Splitter-Größen zurück.
- Model Definition: `PrepareWidget` (min. 596 px, `preparewidget.cpp:411`) mit eigenem Scroll in einem zweiten `QScrollArea` ohne `setWidgetResizable` (`modelwidget.cpp:762,797`) bei `model_tab->setMinimumWidth(300)` (`:607`) -> verschachtelte Scrollbalken (Folge vermutet).
- Fenster: `ResultsDialog`, `StatisticDialog`, `AdvancedSearch`, zwei `ModalDialog` je ModelWidget, alle nicht-modal, `show()/raise()` ohne Geometrie-Merken; Statistik-Dialog wird bei `finished` versteckt (`modelwidget.cpp:676`, `statisticdialog.cpp:962`) während `ResultsDialog::Attention()` erscheint (`:696`).
- Chat-Dock "Enlarge": `setFloating(true)` + 80 %-Geometrie (`external/CuteChat/src/widgets/chatdock.cpp:1439-1462`); Restore setzt nur Geometrie zurück, wenn vorher schwebend.
- More/Less: `ModelActions::ToggleMore` (`widgets/modelactions.cpp:149-172`), fest 0<->100 px, nicht persistent, Animation leakt, Button max. 50 px breit für "..less..", `PushButton` mit festem grünem Stylesheet (`modelactions.h`).

## Arbeitspakete (je ein baubarer Commit)

- **WP0 Test-Gerüst** (zuerst, schützt alle weiteren): `src/tests/test_modelwidget_layout.cpp`, offscreen (`QT_QPA_PLATFORM=offscreen`, Muster `benchmark_mcwidget.cpp:76`).
  - Baut ein ScriptModel-`ModelWidget`, prüft Seitentitel der Liste, keine sichtbaren Top-Level-Fenster nach dem Aufbau (Flacker-Regression), `grab()` nach `docs/ui_redesign/`.
  - Risiko: ui-Quellen liegen in keiner Bibliothek, `tests/CMakeLists.txt` muss sie wie bei `benchmark_mcwidget` einzeln einbinden.
- **WP1 Navigationsliste vervollständigen** (`guitools/sectionstack.h`, `modelwidget.cpp`):
  - Alle Bereiche als Seiten in fester Reihenfolge: Data series (Pin), Model Definition, Parameter Table, Concentration, Statistik / Fit results, Charts, Model Options, System Parameter, Simulated Data (ganz unten, klein). Inventar von Concentration/Charts vor Umsetzung per grep prüfen (Vermutung: heute `ModalDialog` Information/Charts).
  - Seiten, die für das Modell leer sind, erscheinen nicht; Listenbreite 150 px bleibt, Tooltip je Eintrag.
  - Data series wird Seite; wird "angeheftet" (Pin im Seitenkopf), bleibt sie wie heute neben der Seite (Pin-Zustand persistent).
  - Seitenwahl pro Modelltyp bleibt (`model <typ> page`), Schlüssel von Index auf Seitentitel umstellen (Seiten variieren je Modell).
  - Doku: `docs/ASSISTANT_HELP.md` Abschnitt 4 (Tab "Simulated Data", "Parameter Table", Data series) anpassen, Labels aus `tr()` wörtlich.
- **WP2 Model-Definition-Seite entschlacken** (`modelwidget.cpp:758-843`, `preparewidget.cpp`):
  - Äußeres `QScrollArea` entfernen oder `setWidgetResizable(true)`, ein einziger Scrollbereich (`m_field_scroll`).
  - Spaltenzahl der Feldgitter-Zellen aus der Breite ableiten (1 Spalte unter ca. 600 px) statt fest `kColumns = 2`.
  - Abgeleitete Felder (derived) unter "Details" (More/Less aus WP5), Gleichung und lokale Parameter zuerst.
- **WP3 Fenster-Host** (nur für Dialoge, die Fenster bleiben: Scan/AdvancedSearch, Ergebnisse; Statistik und Charts gehen nach WP1 als Seiten in die Liste) (neu `guitools/windowhost.{h,cpp}`, dann ModelWidget/ModalDialog/Statistik/Scan/Results nutzen ihn):
  - Ein Helfer `ShowWindow(QDialog*, key)`: setzt `Qt::Window`, stellt Geometrie aus `QSettings` her, zentriert nur beim allerersten Mal, merkt Geometrie beim Schließen.
  - `StatisticDialog` nicht mehr bei `finished` verstecken; Ergebnisfenster nur anzeigen, wenn kein anderes zugehöriges Fenster das Ziel ist, sonst als Tab.
  - Ziel: Elternfenster = Hauptfenster-Window, nicht das ModelWidget (Position springt sonst mit dem aktiven Tab, Vermutung).
  - Test: Fenster zweimal öffnen/schließen, Geometrie bleibt gleich.
- **WP-F FlowLayout beruhigen** (`guitools/flowlayout.cpp`, 224 Zeilen; Nutzer: `modelwidget.cpp:434` fit_flow, `widgets/modelactions.cpp:66,80`, `modelelement.cpp:72`, `optionswidget.cpp:37`, `systemparameterwidget.cpp:125`, `exportsimulationwidget.cpp:214`, `statisticdialog.cpp:271,475`, `suprafitgui.cpp:332`):
  - Erst Messung: Layout-Durchläufe/Höhenwechsel beim Verkleinern/Vergrößern offscreen protokollieren, Ursache der Unruhe belegen (Vermutung: `heightForWidth` ohne Hysterese, Umbruch an der Kante springt, Mindestgrößen der Items zu groß/zu klein, `PrepareWidget` kommentiert schon das Problem fester Breiten, `preparewidget.cpp:254`).
  - Danach gezielt: stabile `minimumSize`/`sizeHint`, Umbruch erst bei echter Überbreite, Spaltenraster statt freiem Fluss dort, wo die Reihenfolge feststeht (z. B. Fit-Zeile).
  - Test: Breite in Schritten ändern, keine Höhenoszillation, Zeilenzahl monoton.
- **WP5 More/Less-Bauteil** (neu `guitools/morelesstoggle.h`):
  - `QToolButton` mit Pfeil + einheitlichem Text "More"/"Less" (`tr()`), steuert ein Ziel-Widget, Höhe per `sizeHint` statt fester 100 px, Animation mit `DeleteWhenStopped` oder ganz ohne.
  - Zustand pro Schlüssel in `QSettings` (`ui/more/<id>`), Default eingeklappt, bei Offscreen/Test ohne Animation.
  - `ModelActions` umstellen, Stylesheet `#77d740` durch Palette-Rollen ersetzen (Regel: keine Fensterstylesheets).
  - Einsatzorte prüfen: PrepareWidget (derived), ExportSimulationWidget (Advanced-Chips), OptimizerWidget (Tab "Advanced Settings"), Thermogramm ("Advanced stuff").
  - Doku: `docs/ASSISTANT_HELP.md` Zeile 90 ("..more..") auf neue Beschriftung ändern.
- **WP6 advanced_ui vereinheitlichen** (`settingsregistry.cpp:78`, Verbraucher in `statisticdialog.cpp:724`, `modeldataholder.cpp:383`, `exportsimulationwidget.cpp:246-251`, `thermogramwidget.cpp:528`, `suprafitgui.cpp:335`):
  - Tooltip "To apply, please restart" prüfen: `ExportSimulationWidget` reagiert schon live auf `ConfigurationChanged`, die anderen vermutlich nicht.
  - `m_bs` ("Bootstrap") ist immer sichtbar, die übrigen Chips nur mit advanced_ui -> Seite "Simulated Data" ist standardmäßig fast leer; Regel festlegen und anwenden.
- **WP7 Kleine Bildschirme** (1366x768, 1024x600): Mindestgrößen prüfen (`model_tab` 300, Series, `statistic_widget` 140), Orientierungswechsel bei 900 px ohne Größenreset, Renders light/dark in `docs/ui_redesign/`.

## Reihenfolge und Abhängigkeiten

- WP0 -> WP1 -> WP2; WP5 vor WP2 (Details-Einklappen) und WP6; WP3 und WP-F unabhängig davon.
- WP-F am besten früh (vor WP1/WP2), weil WP1 die Seitenbreiten ändert und sonst die Unruhe verdeckt oder verstärkt.
- Pro WP: Build grün, `ctest -R 'Tool|Llm|ModelWidgetLayout'`, Offscreen-Render, `AIChangelog.md`-Zeile, `src/ui/CLAUDE.md` kurz, TODO-Eintrag schließen.

## Risiken

- Gespeicherte Splitter-/Seitenzustände passen nicht zur neuen Struktur: Schlüssel-Version anheben (`layout 5`), wie bei `layout 4`.
- Parentlose Widgets werden mit `setHidden(false)/setVisible(true)` zu Fenstern (Flackern, siehe Commit 98efd118): nur `hide()` oder erst nach Layout-Einbau sichtbar schalten.
- `widgets/modeldataholder.cpp` ist ein veraltetes Duplikat (nicht in `src/ui/CMakeLists.txt`); beim Suchen/Ändern `mainwindow/modeldataholder.cpp` meinen.
