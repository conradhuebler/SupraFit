# Refactoring-Plan: Eliminierung von "Typ C" durch Multi-Projekt-Architektur

**Datum:** 2025-09-01  
**Autor:** Gemini Assistant  
**Status:** Geplant

## 1. Zusammenfassung und Ziel

Dieses Dokument beschreibt den Plan zur vollständigen Abschaffung der "Typ C" (`ml_pipeline`) Datenstruktur in der SupraFit-Codebasis. Das Ziel ist es, die Code-Komplexität drastisch zu reduzieren, die Wartbarkeit zu erhöhen und eine einheitliche, konsistente Datenarchitektur für alle Anwendungsfälle (Standard-Projekte und ML-Pipelines) zu etablieren.

Die "Typ C"-Struktur wird nicht durch einen Patch oder eine Abstraktionsschicht (wie `JsonUtils` allein) behandelt, sondern an der Wurzel eliminiert. Dies wird erreicht, indem die ML-Pipeline so umgestaltet wird, dass sie das bereits vorhandene Multi-Projekt-Dateiformat von SupraFit nutzt.

## 2. Kernstrategie: Eine Datei, viele Projekte

Die Analyse der Funktion `SupraFitGui::SaveAsProjectAction` hat gezeigt, dass SupraFit bereits in der Lage ist, mehrere unabhängige Projekte (jeweils im "Typ A"-Format) in einer einzigen `.suprafit`-Datei unter den Schlüsseln `project_0`, `project_1`, ... zu speichern. Diese Funktionalität wird zum neuen Standard für die Ausgabe der ML-Pipeline.

**Vorteile dieses Ansatzes:**
- **Einheitliches Datenformat:** Es gibt nur noch eine einzige, logische Struktur für Projektdaten.
- **Keine Datei-Flut:** Eine ML-Simulation mit Tausenden von Samples wird in einer einzigen, handhabbaren Datei gespeichert.
- **Maximale Wiederverwendbarkeit:** Alle Werkzeuge und GUI-Komponenten, die mit Standardprojekten arbeiten, sind sofort mit den Ergebnissen der ML-Pipeline kompatibel.
- **Vereinfachtes Debugging:** Jeder einzelne ML-Sample kann als eigenständiges Projekt in der SupraFit-GUI geladen und analysiert werden.

## 3. Phasen des Refactorings

### Phase 1: Umstellung der ML-Pipeline auf das Multi-Projekt-Format

In dieser Phase wird der Output der ML-Pipeline so angepasst, dass er dem neuen Zielformat entspricht.

- **Aktion 1.1: `DataGenerator` anpassen**
  - **Was:** Die Logik, die derzeit die `ml_pipeline`-JSON-Struktur mit dem `fitted_models`-Array erzeugt, wird entfernt.
  - **Neu:** Der `DataGenerator` (oder eine übergeordnete Pipeline-Steuerung) wird modifiziert, um:
    1. Für jeden generierten Datensatz ein vollständiges "Typ A"-Projekt-JSON-Objekt zu erstellen. Dieses Objekt enthält den `data`-Block (mit den synthetischen Daten und Ground-Truth-Metadaten) sowie die `model_X`-Blöcke für jeden gefitteten Kandidaten.
    2. Alle diese Projekt-Objekte in einem Root-JSON-Objekt zu sammeln.
    3. Das Root-Objekt mit den Schlüsseln `project_0`, `project_1`, ... zu strukturieren.
    4. Globale Metadaten des Laufs (wie die `generation_config`) einmalig im Root-Objekt zu speichern.

- **Aktion 1.2: Fitting-Prozess integrieren**
  - **Was:** Der Prozess, der die verschiedenen Modelle auf die generierten Daten fittet, muss seine Ergebnisse (Fit-Qualität, Parameter, Post-Fit-Analyse) direkt in die entsprechende `model_X`-Struktur des jeweiligen `project_X`-Objekts schreiben.
  - **Ziel:** Am Ende von Phase 1 erzeugt die ML-Pipeline eine einzelne, valide `.suprafit`-Datei, die eine Sammlung von Standard-Projekten enthält.

### Phase 2: Anpassung des `MLFeatureExtractor`

Diese Phase vereinfacht den `MLFeatureExtractor` drastisch, indem die gesamte "Typ C"-Parsing-Logik entfernt wird.

- **Aktion 2.1: `parseMLPipelineData` neu implementieren**
  - **Was:** Die Funktion wird so umgeschrieben, dass sie eine Multi-Projekt-Datei erwartet.
  - **Neuer Workflow:**
    1. Lade die übergebene `.suprafit`-Datei.
    2. Iteriere durch die `project_0`, `project_1`, ... Schlüssel im Root-Objekt.
    3. Behandle jedes `project_X`-Objekt als ein eigenständiges "Typ A"-Projekt.
    4. Wende die bestehende (oder leicht angepasste) Logik zur Extraktion von Features aus einem "Typ A"-Projekt an.

- **Aktion 2.2: Redundanten Code entfernen**
  - **Was:** Alle Code-Pfade und Hilfsfunktionen, die sich auf das Parsen von `ml_pipeline`, `fitted_models` oder das Mergen von Root-Modellen mit Pipeline-Daten beziehen, werden ersatzlos gelöscht.
  - **Ziel:** Eine schlanke, wartbare `MLFeatureExtractor`-Klasse, die nur noch ein einziges, klares Datenformat kennt.

### Phase 3: Bereinigung und Dokumentation

In dieser finalen Phase wird die Umstellung abgeschlossen und für die Zukunft festgeschrieben.

- **Aktion 3.1: Globale Code-Analyse**
  - **Was:** Die gesamte Codebasis wird nach verbliebenen Zugriffen auf die alten "Typ C"-Schlüsselwörter (`fitted_models`, `ml_pipeline`) durchsucht und bereinigt.
  - **Ziel:** Sicherstellen, dass keine veraltete Logik zurückbleibt.

- **Aktion 3.2: `JsonUtils` finalisieren**
  - **Was:** Die `JsonUtils`-Klasse wird überprüft und sichergestellt, dass ihre Funktionen (insbesondere `getPostFitAnalysis`) optimal mit der `project_X`-Struktur arbeiten. Sie bleibt ein nützliches Werkzeug für die Arbeit mit den einzelnen Projekt-Objekten.
  - **Ziel:** Eine robuste, universelle Hilfsklasse für den JSON-Zugriff.

- **Aktion 3.3: Dokumentation aktualisieren**
  - **Was:** Die Datei `src/capabilities/SUPRAFIT_JSON_FORMAT.md` (und andere relevante Dokumente) wird überarbeitet.
  - **Inhalt:**
    - Die Multi-Projekt-Struktur (`project_X`) wird als der primäre Standard für Sammlungen von Projekten dokumentiert.
    - Die "Typ C"-Struktur wird als "veraltet" (deprecated) markiert und ihre Verwendung wird nicht mehr empfohlen.
    - Die `JsonUtils`-Klasse wird als die bevorzugte Methode für den Zugriff auf Projektdaten hervorgehoben.
  - **Ziel:** Eine klare, aktuelle und zukunftssichere Dokumentation.

## 4. Verifizierung

- **Manuelle Tests:** Die ML-Pipeline wird ausgeführt, um eine Multi-Projekt-Datei zu erzeugen. Diese Datei wird stichprobenartig mit der SupraFit-GUI geöffnet, um die Korrektheit der einzelnen Projekte zu überprüfen.
- **Funktionstests:** Der `MLFeatureExtractor` wird mit der neuen Dateiart getestet, um sicherzustellen, dass die Feature-Extraktion korrekt funktioniert.
- **Regressionstests:** Die Standard-Funktionalität (Öffnen und Speichern von einzelnen Projekten) wird überprüft, um sicherzustellen, dass keine Nebeneffekte aufgetreten sind.
