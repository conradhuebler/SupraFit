# SupraFit Python Examples and Tests

Diese Verzeichnis enthält Beispiele und Tests für die Python-Schnittstelle von SupraFit.

## Voraussetzungen

1. **SupraFit kompiliert** mit Python-Bindings:
   ```bash
   cd /path/to/SupraFit/build
   cmake .. -DPython_Bindings=ON
   make -j4
   ```

2. **Python 3.6+** installiert

3. **PYTHONPATH** gesetzt (falls nicht installiert):
   ```bash
   export PYTHONPATH=$PYTHONPATH:/path/to/SupraFit/build
   ```

## Verfügbare Tests und Beispiele

### 1. Import-Test (`test_import.py`)

Überprüft, ob das suprafit-Modul korrekt importiert werden kann.

```bash
python3 test_import.py
```

**Was wird getestet:**
- Modul-Import
- Verfügbarkeit der Submodule (io, data, models, statistics)
- Vorhandensein wichtiger Funktionen
- Liste verfügbarer Modelle
- DataClass-Instanziierung

### 2. Grundlegendes Beispiel (`basic_example.py`)

Demonstriert die grundlegende Verwendung der Python-Schnittstelle.

```bash
python3 basic_example.py
```

**Was demonstriert wird:**
- Daten laden
- Modell erstellen
- Modell fitten
- Statistische Analyse
- Ergebnisse exportieren

### 3. Erweiterte Statistik (`advanced_statistics.py`)

Zeigt erweiterte statistische Analysen.

```bash
python3 advanced_statistics.py
```

**Was demonstriert wird:**
- Monte Carlo Unsicherheitsanalyse
- Cross-Validation
- Konfidenzintervalle
- Modellvergleich mit AIC
- Umfassende Ergebnisexporte

### 4. Integration Test (`test_integration.py`)

**Neu!** Umfassender Integrationstest mit echten SupraFit-Testdaten.

```bash
python3 test_integration.py
```

**Was wird getestet:**
- Laden von echten NMR-Daten (`input/1_1_1_2_001.dat`)
- Modell-Erstellung und -Fitting
- Statistische Analysen
- Export-Funktionen
- Persistenz (Speichern/Laden)

**Ausgabe:**
```
✓ PASS: Module import and version check
✓ PASS: Submodule availability
✓ PASS: Available models list
✓ PASS: DataClass creation
✓ PASS: Load NMR test data
✓ PASS: Create NMR model
✓ PASS: Set model parameters
✓ PASS: Fit model
✓ PASS: Statistical summary
✓ PASS: Monte Carlo (quick)
✓ PASS: Export results
✓ PASS: Save/Load model

Results: 12/12 tests passed (100.0%)
```

### 5. Test mit Testdaten (`test_with_testdata.py`)

**Neu!** Realistischer Test mit den gleichen Daten wie die C++ CLI-Tests.

```bash
python3 test_with_testdata.py
```

**Was wird getestet:**
- Laden von `input/1_1_1_2_001.dat` (NMR 1:1+1:2 Titrationsdaten)
- NMR 1:1+1:2 Modell erstellen
- Parameter setzen und optimieren
- Statistische Zusammenfassung
- Monte Carlo Analyse (100 Iterationen)
- Export nach CSV und TXT
- Modell als JSON speichern

**Ausgabebeispiel:**
```
✓ Loaded successfully
  Data points: 77
  Series count: 7
  Independent variables: 2

✓ Model created: NMR 1:1+1:2
  Global parameters: 2
  Local parameters: 14

✓ Fit successful!
  SSE: 0.003421
  SEy: 0.000089

✓ Exported CSV to: /tmp/suprafit_python_test_results.csv
✓ Saved model to: /tmp/suprafit_python_test_model.json
```

## Verwendete Testdaten

Die Tests verwenden echte Daten aus dem `input/` Verzeichnis:

- **`1_1_1_2_001.dat`**: NMR-Titrationsdaten für 1:1 + 1:2 Bindungsmodell
  - 77 Datenpunkte
  - 7 Serien (Chemical Shifts)
  - 2 unabhängige Variablen (Konzentrationen)

Diese Daten sind die gleichen, die auch von der C++ CLI verwendet werden, um Konsistenz zu gewährleisten.

## Schnellstart

**1. Nur Import testen:**
```bash
python3 test_import.py
```

**2. Vollständiger Integrationstest:**
```bash
python3 test_integration.py
```

**3. Realistisches Szenario:**
```bash
python3 test_with_testdata.py
```

## Fehlerbehebung

### Import-Fehler: `ModuleNotFoundError: No module named 'suprafit'`

**Lösung 1:** PYTHONPATH setzen
```bash
export PYTHONPATH=$PYTHONPATH:/path/to/SupraFit/build
```

**Lösung 2:** Modul installieren
```bash
cd /path/to/SupraFit/build
sudo make install
```

**Lösung 3:** In Build-Verzeichnis ausführen
```bash
cd /path/to/SupraFit/build
python3 ../examples/python/test_import.py
```

### Test-Fehler: "File not found: input/1_1_1_2_001.dat"

Die Tests erwarten, dass sie aus dem `examples/python/` Verzeichnis ausgeführt werden.

```bash
cd /path/to/SupraFit/examples/python
python3 test_integration.py
```

### Kompilierungsfehler: Qt6 nicht gefunden

Stellen Sie sicher, dass Qt6 installiert ist:

```bash
# Ubuntu/Debian
sudo apt-get install qt6-base-dev qt6-qml-dev

# Fedora/RHEL
sudo dnf install qt6-qtbase-devel qt6-qtdeclarative-devel

# macOS
brew install qt@6
```

## Vergleich mit C++ CLI

Die Python-Schnittstelle bietet die gleiche Funktionalität wie `suprafit_cli`:

| Funktion | C++ CLI | Python |
|----------|---------|--------|
| Daten laden | `--load file.dat` | `sf.io.load_data("file.dat")` |
| Modell fitten | JSON-Konfiguration | `sf.models.fit_model(model)` |
| Monte Carlo | JSON-Task | `sf.statistics.monte_carlo(model)` |
| Export | JSON-Output | `sf.io.export_results(model)` |

Die Python-Schnittstelle ist **einfacher zu verwenden** und ermöglicht **interaktive Analyse** in Jupyter Notebooks.

## Integration in eigene Skripte

```python
import suprafit as sf

# Daten laden
data = sf.io.load_data("my_experiment.txt")

# Modell erstellen und fitten
model = sf.models.create_model("nmr_1_1", data)
result = sf.models.fit_model(model)

# Statistische Analyse
mc = sf.statistics.monte_carlo(model, iterations=10000)

# Ergebnisse exportieren
sf.io.export_results(model, "results.csv", "csv")
```

## Performance-Hinweise

Die Tests verwenden reduzierte Iterationszahlen für schnelle Ausführung:

- **Quick Tests**: 100 Monte Carlo Iterationen (~5 Sekunden)
- **Produktion**: 10000+ Iterationen empfohlen (~1-2 Minuten)

Für Produktions-Analysen:
```python
mc = sf.statistics.monte_carlo(model, iterations=10000, confidence=0.95)
```

## Weitere Informationen

- **Python Interface Dokumentation**: `../../docs/PYTHON_INTERFACE.md`
- **C++ Dokumentation**: `../../src/python_bindings/README.md`
- **SupraFit Hauptdokumentation**: `../../CLAUDE.md`

## Support

Bei Problemen:
1. Führen Sie `test_import.py` aus, um die Installation zu verifizieren
2. Überprüfen Sie die Build-Logs auf Fehler
3. Konsultieren Sie `../../src/python_bindings/CLAUDE.md` für technische Details
