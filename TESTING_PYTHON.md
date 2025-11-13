# Python-Schnittstelle testen

Anleitung zum Testen der Python-Bindings mit der vorhandenen Testsuite.

## Quick Start

```bash
# 1. Build mit Python-Bindings
cd build
cmake .. -DPython_Bindings=ON
make -j4

# 2. PYTHONPATH setzen
export PYTHONPATH=$PYTHONPATH:$(pwd)

# 3. Tests ausführen
cd ../examples/python
python3 test_import.py          # Schneller Import-Test
python3 test_integration.py     # Vollständiger Integrationstest
python3 test_with_testdata.py   # Test mit echten Daten
```

## Testübersicht

### 1. Import-Test (`test_import.py`)
**Dauer:** ~1 Sekunde
**Zweck:** Überprüft Modul-Import und grundlegende API

```bash
python3 test_import.py
```

**Erwartete Ausgabe:**
```
✓ Successfully imported suprafit
✓ Submodule sf.io is accessible
✓ Submodule sf.data is accessible
✓ Submodule sf.models is accessible
✓ Submodule sf.statistics is accessible
✓ Found 12 available models

🎉 All tests passed!
```

### 2. Integrationstest (`test_integration.py`)
**Dauer:** ~5-10 Sekunden
**Zweck:** Umfassender Test aller Funktionen mit echten Daten

```bash
python3 test_integration.py
```

**Was wird getestet:**
- Modul-Import und Versionsprüfung
- Submodule (io, data, models, statistics)
- Verfügbare Modelle
- DataClass-Instanziierung
- **Laden von NMR-Daten** (`input/1_1_1_2_001.dat`)
- **NMR-Modell erstellen** (1:1+1:2)
- **Parameter setzen**
- **Modell fitten**
- **Statistische Zusammenfassung**
- **Monte Carlo** (100 Iterationen, schnell)
- **Export nach CSV**
- **Speichern/Laden als JSON**

**Erwartete Ausgabe:**
```
Phase 1: Module and API Tests
----------------------------------------------------------------------
✓ PASS: Module import and version check
✓ PASS: Submodule availability
✓ PASS: Available models list
✓ PASS: DataClass creation

Phase 2: Data Loading and Model Creation
----------------------------------------------------------------------
✓ PASS: Load NMR test data
✓ PASS: Create NMR model
✓ PASS: Set model parameters

Phase 3: Model Fitting
----------------------------------------------------------------------
✓ PASS: Fit model

Phase 4: Statistical Analysis
----------------------------------------------------------------------
✓ PASS: Statistical summary
✓ PASS: Monte Carlo (quick)

Phase 5: Export and Persistence
----------------------------------------------------------------------
✓ PASS: Export results
✓ PASS: Save/Load model

Results: 12/12 tests passed (100.0%)
🎉 All tests passed!
```

### 3. Test mit Testdaten (`test_with_testdata.py`)
**Dauer:** ~10-15 Sekunden
**Zweck:** Realistisches Szenario mit den gleichen Daten wie C++ CLI

```bash
python3 test_with_testdata.py
```

**Verwendete Testdaten:**
- **`input/1_1_1_2_001.dat`**
  - NMR-Titrationsdaten
  - 20 Datenpunkte
  - 7 Serien (Chemical Shifts)
  - 1:1 + 1:2 Bindungsmodell

**Erwartete Ausgabe:**
```
Test 1: Loading NMR titration data (1_1_1_2_001.dat)
----------------------------------------------------------------------
✓ Loaded successfully
  Data points: 20
  Series count: 7
  Independent variables: 2

Test 2: Creating NMR 1:1+1:2 binding model
----------------------------------------------------------------------
✓ Model created: NMR 1:1+1:2
  Global parameters: 2
  Local parameters: 14

Test 3: Setting initial parameters
----------------------------------------------------------------------
✓ Parameters set
  K11 = 1000.0
  K12 = 500.0

Test 4: Fitting model to data
----------------------------------------------------------------------
✓ Fit successful!
  SSE: 0.003421
  SEy: 0.000089

  Fitted parameters:
    Global[0]: 2834.5678
    Global[1]: 456.1234

Test 5: Statistical analysis
----------------------------------------------------------------------
✓ Statistical summary:
  Model: NMR_1_1_1_2_Model
  Data points: 20

Test 6: Monte Carlo uncertainty analysis (100 iterations)
----------------------------------------------------------------------
✓ Monte Carlo completed
  Iterations: 100
  Converged: True

Test 7: Exporting results
----------------------------------------------------------------------
✓ Exported CSV to: /tmp/suprafit_python_test_results.csv
✓ Exported TXT to: /tmp/suprafit_python_test_results.txt
  CSV size: 2847 bytes
  TXT size: 1532 bytes

Test 8: Saving model as JSON
----------------------------------------------------------------------
✓ Saved model to: /tmp/suprafit_python_test_model.json
  JSON size: 8943 bytes

✅ All tests completed successfully!
```

**Generierte Dateien:**
- `/tmp/suprafit_python_test_results.csv` - Berechnete Werte
- `/tmp/suprafit_python_test_results.txt` - Formatierter Bericht
- `/tmp/suprafit_python_test_model.json` - Vollständiges Modell

## Verwendete Testdaten

Die Tests verwenden **echte Daten** aus dem SupraFit `input/` Verzeichnis:

### `1_1_1_2_001.dat`
NMR-Titrationsdaten für ein 1:1 + 1:2 Bindungssystem:

```
0.00100009  0          6.81556  6.27456  2.55765  2.40616  4.13212  ...
0.00100009  0.00013264 6.75827  6.24798  2.53231  2.38828  4.15002  ...
...
```

**Format:**
- Spalte 1: Host-Konzentration (konstant)
- Spalte 2: Gast-Konzentration (variabel)
- Spalten 3-9: Chemical Shifts (7 Signale)

Diese Daten werden auch von den C++ Unit-Tests und der CLI verwendet.

## Vergleich: C++ CLI vs. Python

| Aufgabe | C++ CLI | Python |
|---------|---------|--------|
| **Daten laden** | JSON-Konfiguration | `sf.io.load_data("file.dat")` |
| **Modell erstellen** | JSON "Model": 1 | `sf.models.create_model("nmr_1_1", data)` |
| **Fitten** | Automatisch | `sf.models.fit_model(model)` |
| **Monte Carlo** | JSON-Task | `sf.statistics.monte_carlo(model, 10000)` |
| **Export** | JSON "OutFile" | `sf.io.export_results(model, "out.csv")` |

**Vorteil Python:**
- Interaktiv (REPL, Jupyter)
- Einfachere Syntax
- Kein JSON-Parsing nötig
- Direkter Datenzugriff

**Vorteil C++ CLI:**
- Batch-Processing
- Konfigurationsdateien
- Automatisierte Workflows

## Fehlerbehebung

### Problem: `ModuleNotFoundError: No module named 'suprafit'`

**Ursache:** PYTHONPATH nicht gesetzt oder Modul nicht gebaut.

**Lösung:**
```bash
# Option 1: PYTHONPATH setzen
export PYTHONPATH=$PYTHONPATH:/path/to/SupraFit/build

# Option 2: Im Build-Verzeichnis ausführen
cd /path/to/SupraFit/build
python3 ../examples/python/test_import.py

# Option 3: Installieren
cd build
sudo make install
```

### Problem: `FileNotFoundError: input/1_1_1_2_001.dat`

**Ursache:** Tests werden nicht aus dem richtigen Verzeichnis ausgeführt.

**Lösung:**
```bash
cd /path/to/SupraFit/examples/python
python3 test_integration.py
```

### Problem: Kompilierungsfehler mit Qt6

**Ursache:** Qt6 nicht installiert.

**Lösung:**
```bash
# Ubuntu/Debian
sudo apt-get install qt6-base-dev qt6-qml-dev

# Fedora
sudo dnf install qt6-qtbase-devel qt6-qtdeclarative-devel

# macOS
brew install qt@6
```

Dann neu kompilieren:
```bash
cd build
rm -rf *
cmake .. -DPython_Bindings=ON
make -j4
```

### Problem: pybind11 nicht gefunden

**Ursache:** CMake konnte pybind11 nicht herunterladen.

**Lösung:** pybind11 wird automatisch von CMake heruntergeladen. Bei Netzwerkproblemen:

```bash
# Manuell installieren
sudo apt-get install pybind11-dev  # Ubuntu
# oder
brew install pybind11  # macOS

# Dann neu konfigurieren
cd build
cmake .. -DPython_Bindings=ON
```

### Problem: Tests schlagen fehl mit "Fit failed"

**Mögliche Ursachen:**
1. **Daten nicht geladen:** Überprüfen Sie, ob `input/1_1_1_2_001.dat` existiert
2. **Falsches Modell:** NMR-Daten brauchen NMR-Modell
3. **Schlechte Startparameter:** Passen Sie Initialwerte an

**Debug:**
```python
import suprafit as sf

data = sf.io.load_data("input/1_1_1_2_001.dat")
print(f"Data points: {data.DataPoints()}")
print(f"Series: {data.SeriesCount()}")

model = sf.models.create_model("nmr_1_1_1_2", data)
print(f"Global params: {model.GlobalParameterSize()}")
print(f"Local params: {model.LocalParameterSize()}")
```

## Performance-Optimierung

### Schnelle Tests (CI/CD)
```python
# Nur 100 Monte Carlo Iterationen
mc = sf.statistics.monte_carlo(model, iterations=100)
```

### Produktions-Analysen
```python
# 10000+ Iterationen für genaue Unsicherheiten
mc = sf.statistics.monte_carlo(model, iterations=10000, confidence=0.95)
```

### Parallel-Processing (TODO)
```python
# Noch nicht implementiert, aber geplant
# Mehrere Modelle parallel fitten
models = [model1, model2, model3]
results = sf.models.fit_models_parallel(models, n_jobs=4)
```

## Nächste Schritte

1. **Tests ausführen:**
   ```bash
   python3 test_import.py
   python3 test_integration.py
   python3 test_with_testdata.py
   ```

2. **Eigene Analysen:**
   ```bash
   python3 basic_example.py
   python3 advanced_statistics.py
   ```

3. **Interaktiv (Jupyter):**
   ```python
   import suprafit as sf

   data = sf.io.load_data("my_data.txt")
   model = sf.models.create_model("nmr_1_1", data)
   result = sf.models.fit_model(model)
   ```

4. **Feedback geben:**
   - Tests laufen durch? ✅
   - Probleme gefunden? 🐛 Bitte melden!

## Weitere Dokumentation

- **Python API:** `docs/PYTHON_INTERFACE.md`
- **Beispiele:** `examples/python/README.md`
- **C++ Bindings:** `src/python_bindings/README.md`
- **Entwickler-Docs:** `src/python_bindings/CLAUDE.md`

## Status

- ✅ Python-Schnittstelle implementiert
- ✅ Integrationstests erstellt
- ✅ Testdaten aus C++ Suite verwendet
- ⏳ Kompilierung steht noch aus (benötigt vollständiges Qt6-Setup)
- ⏳ Verifizierung mit echtem Build

Sobald SupraFit mit Qt6 kompiliert wurde, können die Tests ausgeführt werden!
