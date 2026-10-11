# Lost and Found: Eine To-Do-Liste aus den Archiven

Dieses Dokument enthält eine kuratierte Liste von unvollendeten Aufgaben, geplanten Funktionen und guten Ideen, die in der Projektdokumentation, insbesondere im Verzeichnis `docs/outdated`, entdeckt wurden. Es dient als "Fundgrube" für wertvolle Konzepte, die geplant, aber nie vollständig umgesetzt wurden.

## 1. Architektur und Refactoring

### 1.1. Vereinheitlichung der Kernlogik von CLI und UI
-   **Aufgabe**: Die Konsolidierung der Kernlogik von CLI und GUI abschließen, um die Duplizierung von Code zu beseitigen.
-   **Details**:
    -   **Phase 1: ✅ COMPLETED (Nov 3 2025)**: CLI ProjectManager Migration
        - `PrintFileStructure()` migriert zu ProjectManager API
        - 24 legacy TODOs zu MIGRATION POINT konvertiert
        - Foundation für tiefere CLI-Refaktoring gelegt
    -   **Phase 2: In Progress - Error Handling verbessert**: Error Handling in AnalysisManager implementiert
        - Try-catch Blöcke um JobManager::RunJobs() hinzugefügt
        - Error Signals für Fehlerbehandlung emittiert
        - Graceful error recovery: fehlgeschlagene Methoden überspringen
        - **Nächste Schritte**: `TaskController` erstellen - eine einheitliche, UI-unabhängige Komponente zur Ausführung von Aufgaben wie Modellanpassung und statistischer Analyse. Der `TaskController` würde ein Modell und eine Job-Konfiguration als Eingabe nehmen und den `JobManager` sowie den `Minimizer` kapseln.
    -   **Phase 3: Pending - Daten-Generierung vereinheitlichen**: Schaffung einer High-Level-Schnittstelle im Kern für Aufgaben der Datengenerierung, möglicherweise durch Erweiterung des `ProjectManager` oder Erstellung einer `DataFactory`-Klasse.
-   **Quelle**: `docs/CLI_UI_CONSOLIDATION_PLAN.md`

### 1.2. Refactoring von `analyse.cpp` abschließen
-   **Aufgabe**: Das Refactoring von `src/core/analyse.cpp` beenden, um die `JsonUtils`-Klasse vollständig zu nutzen.
-   **Details**: Die Datei enthält derzeit eine Mischung aus alten, string-basierten Analysefunktionen (`AnalyseReductionAnalysis`, `CompareCV`, etc.) und neuen, JSON-basierten `Calculate...Metrics`-Funktionen. Die alten Funktionen sollten ausgemustert und entfernt werden, um den Übergang zu einem einheitlichen Datenzugriffsmuster abzuschließen.
-   **Quelle**: Analyse von `docs/outdated/REFACTORING_ANALYSIS.md` und `src/core/analyse.cpp`.

### 1.3. Verbesserung der Kern-Klassenstruktur
-   **Aufgabe**: Das geplante Refactoring der Kern-Analyseklassen implementieren.
-   **Details**:
    -   Zusammenführung der Klassen `MonteCarloStatistics` und `ModelComparison` zu einer einzigen, allgemeineren `MonteCarloAnalysis`-Klasse.
    -   Aufteilung der `ResampleAnalyse`-Klasse in zwei separate Klassen: `CrossValidationAnalysis` und `ReductionAnalysis`.
-   **Quelle**: `docs/outdated/COMPREHENSIVE_REFACTORING_PLAN.md`

### 1.4. Entscheidung über die Zukunft der `ml_pipeline`-Struktur
-   **Aufgabe**: Das Refactoring zur Beseitigung der alten "Typ C" (`ml_pipeline`) JSON-Struktur ist unvollständig. Eine Entscheidung muss getroffen werden.
-   **Details**: Die neue Multi-Projekt-Architektur wurde implementiert, aber die alte `ml_pipeline`-Funktionalität wurde nicht entfernt. Das Team sollte entscheiden, ob die Migration abgeschlossen und der alte Code entfernt wird oder ob beide Strukturen offiziell unterstützt werden sollen. Die Dokumentation muss entsprechend dieser Entscheidung aktualisiert werden.
-   **Quelle**: Analyse von `docs/outdated/REFACTORING_PLAN_TYPE_C.md`.

## 2. UI/UX-Verbesserungen

### 2.1. Überarbeitung des Hauptfensters und der Dialoge
-   **Aufgabe**: Refactoring des Hauptfensters und der Dialoge zur Verbesserung der Konsistenz und Wartbarkeit.
-   **Details**:
    -   **✅ COMPLETED (Nov 3 2025)**: Entfernung von deprecated Code
        - Legacy `SetData()` Methode aus MainWindow gelöscht
        - Header bereinigt - erzwingt ProjectManager-Nutzung
        - Alle GUI-Code nutzt `setDataFromProjectManager()`
    -   **TODO**: Erstellung einer `Dialog`-Basisklasse, um ein einheitliches Erscheinungsbild zu gewährleisten.
    -   **TODO**: Refactoring der Klassen `ImportData`, `ResultsDialog` und `StatisticDialog`.
    -   **TODO**: Weitere Aufräumen der Klassen `SupraFitGui` und `MainWindow` durch Aufteilung in kleinere Komponenten (z.B. `ViewManager`, `MenuManager`) und Entfernung von verbleibendem Dead Code.
-   **Quelle**: `docs/REFACTORING_DIALOGS.md`, `docs/REFACTORING_MAIN_WINDOW.md`

### 2.2. Umsetzung von UI-Verbesserungen aus Benutzerfeedback
-   **Aufgabe**: Die konkreten UI-Verbesserungen aus dem Benutzerfeedback umsetzen.
-   **Details**:
    -   Ersetzen der überladenen Symbolleisten durch eine strukturierte, hierarchische Menüleiste (`Data`, `Model`, `Evaluation`).
    -   Verschieben des Nachrichtenprotokolls in ein eigenes Panel auf der linken Seite, neben der Projektansicht.
    -   Parameterfelder im `ModelWidget` nach einer erfolgreichen Anpassung schreibgeschützt machen.
    -   Verbesserung der Anzeige von lokalen Parametern im `ModelWidget` durch eine strukturiertere Ansicht wie eine `QTableView`.
-   **Quelle**: `docs/UI_IMPROVEMENT_PLAN.md`

### 2.3. Optimierung der UI-Performance und Speicherverwaltung
-   **Aufgabe**: Die geplanten Performance-Optimierungen für die UI implementieren.
-   **Details**:
    -   In `ModelDataHolder` die redundante `m_models`-Liste eliminieren.
    -   In `ProjectTree` den rohen Zeiger `m_data_list` durch einen `QSharedPointer` ersetzen und für Aktualisierungen auf einen Signal/Slot-Mechanismus umstellen, anstatt den gesamten Baum neu zu erstellen.
-   **Quelle**: `docs/outdated/ACTIONABLE_REFACTORING_PLAN.md`, `docs/REFACTORING_MAIN_WINDOW.md`

## 3. Neue Funktionen und Erweiterungen

### 3.1. Implementierung der Python-Unterstützung
-   **Aufgabe**: Die geplante Unterstützung für Python-Skripte implementieren.
-   **Details**: Die ersten Arbeiten wurden begonnen, aber die Funktion wurde nie fertiggestellt. Dies würde wahrscheinlich die Integration eines Python-Interpreters und das Verfügbarmachen der notwendigen SupraFit-Objekte und -Funktionen beinhalten.
-   **Quelle**: `docs/Python.md`

### 3.2. Verbesserung der geskripteten Modelle
-   **Aufgabe**: Die Implementierung von geskripteten Modellen verbessern.
-   **Details**:
    -   Erstellung einer konsistenten API (`ScriptingEngine`-Basisklasse), um die verschiedenen Skript-Backends (ChaiScript, Duktape, etc.) zu abstrahieren.
    -   Untersuchung von Performance-Optimierungen für jedes Backend (z.B. Bytecode-Kompilierung).
    -   Erweiterung der Funktionalität zur Unterstützung von Modellen mit mehreren Ausgabespalten.
    -   Aktualisierung der Dokumentation mit klaren Beispielen für jede Engine.
-   **Quelle**: `docs/REFACTORING_SCRIPTED_MODELS.md`

## 4. Fehlerbehebung und Wartung

### 4.1. Behebung des GUI-Bugs beim Laden von Array-Strukturen
-   **Aufgabe**: Behebung des kritischen Fehlers, der beim Laden eines Projekts mit einer Array-basierten `methods`-Struktur zu einer Modellverdopplung in der GUI führt.
-   **Details**: Dieser Fehler verhindert die korrekte Anzeige von mehreren Post-Processing-Analyse-Läufen. Das Dokument `KNOWN_ISSUES.md` enthält eine detaillierte Analyse des Problems und eine vorgeschlagene Lösungsstrategie.
-   **Quelle**: `docs/KNOWN_ISSUES.md`

### 4.2. Bereinigung der Dokumentation
-   **Aufgabe**: Konsolidierung und Korrektur der Projektdokumentation.
-   **Details**:
    -   **✅ PARTIALLY COMPLETED (Nov 3 2025)**: Dokumentation konsolidiert
        - `docs/outdated/README.md` mit korrigierten Status-Aussagen aktualisiert
        - Unterschiedliche Status zwischen Original und realer Implementierung dokumentiert
        - `lostandfound.md` erstellt als Fundgrube für unvollendete Aufgaben
        - `TODO.md` aktualisiert mit completion status der Tasks
    -   **TODO**: Die Dokumentation zur Multi-Run-Implementierung ist widersprüchlich. Sie sollte zu einer einzigen, genauen Beschreibung vereinheitlicht werden.
    -   **TODO**: Das Verzeichnis `docs/outdated` könnte noch gründlicher analysiert werden für weitere aufgedeckte Fehler.
-   **Quelle**: Allgemeine Analyse des `docs`-Verzeichnisses.
