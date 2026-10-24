<!--
Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
Claude Generated - work package / roadmap document
-->

# Work Package: Add-Model-Leiste (NMR als Kern, Stöchiometrien gebündelt, nach Häufigkeit)

**Status:** PLAN, Entscheidungen des Operators eingearbeitet (nichts umgesetzt) · **Branch:** `feature/add-model-bar` (baut auf `feature/model-pane-navigation` auf)

## Ziel

- Modelle hinzufügen ist die häufigste Handlung im Workspace und bekommt eine **eigene Leiste**, die man andocken/lösen kann (QToolBar, alle Seiten, Zustand wird mit dem Fenster gespeichert).
- **Technik und Stöchiometrie getrennt:** eine einzige Stöchiometrie-Liste (1:1, 1:1 / 1:2, 2:1 / 1:1, 2:1 / 1:1 / 1:2, …) statt je Technik dieselbe Liste noch einmal; die **Technik** (NMR, UV/Vis, Fluoreszenz, Kalorimetrie) wird daneben gewählt. **Default: NMR**, änderbar durch Bedienung und gezielt per Einstellung.
- Die Leiste füllt sich **nach Häufigkeit** der Aufrufe: die meistgenutzten Kombinationen stehen vorn.

## Entscheidungen des Operators

- Die Technik-Wahl besteht aus **Knöpfen nebeneinander** (NMR | UV/Vis | Fluoreszenz | Kalorimetrie), nicht aus einer Auswahlbox.
- **Inhalt der Schnellleiste wählbar:** entweder die **x häufigsten** (Zähler, x einstellbar) oder eine **feste, vom Operator zusammengestellte** Liste; die Betriebsart ist eine Einstellung.
- **Default-Technik = zuletzt benutzte**; das kann später verfeinert werden (iterieren).
- Das Menü "Model" **bleibt** als Zugriff auf alles.
- Auch die Stöchiometrien jenseits der vier festen (Host-Dimerisierung, Dreikomponenten) und **eigene Modelle** (Script-Presets, gespeicherte Modelldateien, ein eigenes Modell mit Gleichgewichtssystem) können in die Schnellleiste, **nach Häufigkeit oder nach Wunsch** (editierbar).
- Der gemeldete Fehler "eigenes Modell mit Gleichgewichtssystem anlegen, dann eines heraussuchen" wurde gefunden und behoben (Presets und eigene Gleichungen für weniger Eingaben als die Daten Spalten haben, 2026-10-24, `ScriptModel::PresetBlockFor`, `PrepareWidget::setInputColumns`); der Bar-Umbau setzt darauf auf.

## Ist-Stand (Belege)

- Menü "Model" der Toolbar (`modeldataholder.cpp:320-395`): "New Model" (Script-Presets), "Load Model File…", je Technik ein Untermenü (`m_nmr_menu`, `m_uv_vis_menu`, `m_fl_menu`, `m_itc_menu`, `m_kinetics_menu`), gefüllt von `MDHDockTitleBar::addToMenu(IndependetCount)`: 1 Eingabespalte = Kalorimetrie/Kinetik, 2 = NMR/UV-Vis/Fluoreszenz.
- Je Technik dieselben vier festen Modelle (z. B. `nmr_ItoI`, `nmr_IItoI_ItoI`, `nmr_ItoI_ItoII`, `nmr_IItoI_ItoI_ItoII`; `fl_*`, `uv_vis_*`, `itc_*` analog) plus ein `*_any`-Modell mit Reaktions-Presets (`ReactionEditorWidget::Presets()`), das ohne Preset den Dialog "Define Model" öffnet.
- Hinzufügen läuft über `MDHDockTitleBar::AddModel` (`m_model_choosen`, `m_preset_reactions`) -> `ModelDataHolder::AddModel(int, QString)`.
- Die Hauptleiste (`SupraFitGui::SetupToolBar`) ist nicht verschiebbar; der Fensterzustand (`restoreState`) wird bereits gespeichert.

## Vorschlag

- **Katalog** (rein, testbar): Technik x Stöchiometrie -> `{festes Modell | any-Modell + Preset}`; welche Techniken zu den Daten passen (Eingabespalten); Eintragsarten: Stöchiometrie, Script-Preset, Modelldatei.
- **Leiste** `AddModelBar` (QToolBar, `objectName` für den gespeicherten Zustand, alle Seiten, schwebend): links die Technik als Knöpfe nebeneinander (nur die passenden), dann die Schnellleiste mit Knöpfen für die gewählten Einträge, rechts "More..." (alles, wie das Menü) und "Edit...". Ein Klick fügt das Modell der gewählten Technik hinzu; ein Eintrag einer anderen Art (Script-Preset, Modelldatei) ist von der Technik unabhängig.
- **Schnellleiste füllen:** Modus "häufigste" (Zähler je Eintrag, die x häufigsten, x einstellbar, stabile Reihenfolge bei Gleichstand, Zähler rücksetzbar) oder "feste Liste" (Rechtsklick "Anpassen" an jedem Eintrag in Menü/Leiste, Dialog "Schnellleiste anpassen": Einträge mit Häkchen und Reihenfolge).
- **Default-Technik:** zuletzt benutzte; eine Einstellung kann sie auf "immer NMR" (oder eine andere) festlegen.

## Änderung des Entwurfs (Operator, nach WP-C)

- Die Hauptleiste wird in **drei Zonen** geteilt: links Neu/Öffnen/Speichern und die Menüs, **in der Mitte die Modelle**, rechts SupraFit, Chat (Agent) und Log. Die Add-Model-Leiste ist damit ein Widget in der Mitte der Hauptleiste, keine eigene Leiste mehr (Andocken/Schweben entfällt).
- Die **Technik ist kein Knopfreihe** mehr, sondern gehört zum Eintrag: ein Klick auf "1:1" fügt das Modell der Technik hinzu, mit der der Eintrag am häufigsten benutzt wurde (NMR, solange nie benutzt), der Pfeil daneben öffnet NMR / UV/Vis / Fluoreszenz ...

## Arbeitspakete (je ein baubarer Commit)

- **WP-A Katalog (erledigt, `modelcatalog.{h,cpp}`, 5 Tests `catalog*`):** `mainwindow/modelcatalog.{h,cpp}`; Test: jede (Technik, Stöchiometrie) liefert ein Modell, das sich mit passenden Daten definieren lässt und dieselben Parameter hat wie der Weg über das Menü; Eintragsarten.
- **WP-B Zähler und Auswahl (erledigt, `modelusage.{h,cpp}`, 4 Tests `usage*`; die Betriebsart/x/feste Liste liegen in `ModelUsage::Preferences`, der Dialog dazu ist WP-D):** Aufrufzähler je Eintrag (`QSettings` `model_usage/...`), Modus häufigste/feste Liste, x, Zurücksetzen; Test mit Zählern und stabiler Reihenfolge.
- **WP-C Leiste (erledigt, `addmodelbar.{h,cpp}`, 3 Tests `addModelBar*`; Zahnrad: Betriebsart und Zähler zurücksetzen, die feste Liste bearbeiten kommt mit WP-D):** `AddModelBar`, eingebunden in `SupraFitGui` (folgt dem Projekt auf dem Bildschirm wie die Workspace-Menüs, aus ohne Projekt), schwebend/andockbar, Offscreen-Test und Render hell/dunkel.
- **WP-D Anpassen (erledigt: Dialog, Zahnrad, Rechtsklick-Menü der Knöpfe, `quickbardialog.{h,cpp}`, 2 Tests; "in die Schnellleiste" im Menü "Model" kommt mit WP-E; die Einstellungs-Registry nimmt die Werte nicht auf, sie liegen in `QSettings` `add_model_bar/*`):** Dialog und Rechtsklick-Menü für die feste Liste, Einstellung in der Registry (`add_model_bar_mode`, `add_model_bar_count`, `add_model_technique`).
- **WP-E Menü "Model" (erledigt, abgewandelt: das Menü bleibt wie es ist, jeder Eintrag wird über `ModelCatalog::Identify` dem Katalog zugeordnet; Zählen und "Add to the quick bar" per Rechtsklick; das Menü selbst nicht aus dem Katalog gebaut, die Listen dort bleiben bis zur Menü-Umstellung doppelt):** bleibt der Zugriff auf alles.
- **WP-F Hilfe:** `docs/ASSISTANT_HELP.md`, Tooltips mit voller Bezeichnung.

## Risiken

- Die festen Modelle und `*_any` unterscheiden sich in Parametern/Dialog (`DemandInput`): der Katalog muss pro Technik sagen, welcher Weg gilt; Fluoreszenz/UV-Vis haben nicht alle Presets.
- Die Behebung des Spaltenproblems steht noch unter Beobachtung (`TODO.md`): der Operator bestätigt, ob es der gemeldete Fehler war.
- Eine verschiebbare Leiste braucht einen stabilen `objectName`, sonst geht der gespeicherte Zustand verloren.
