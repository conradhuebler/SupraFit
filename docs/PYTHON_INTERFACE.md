# SupraFit Python Interface

## Overview

Eine moderne Python-Schnittstelle für SupraFit, die es ermöglicht:
- **Daten zu laden** und zu manipulieren
- **Modelle zu erstellen** und zu fitten (NMR, ITC, Fluoreszenz, UV-Vis)
- **Statistische Analysen** durchzuführen (Monte Carlo, Cross-Validation, AIC)
- **Ergebnisse zu exportieren** in verschiedenen Formaten

## Installation

### Voraussetzungen
- Python 3.6 oder höher
- Qt6 (Core und Qml)
- CMake 3.21+
- SupraFit C++ Bibliotheken (core, models)

### Build

```bash
cd /path/to/SupraFit
mkdir -p build && cd build

# Konfigurieren mit Python-Bindings
cmake .. -DPython_Bindings=ON

# Kompilieren
make -j4

# Optional: Installieren
sudo make install
```

Das Python-Modul wird automatisch gebaut und nach `build/` kopiert.

### Testen ohne Installation

```bash
cd /path/to/SupraFit/build
export PYTHONPATH=$PYTHONPATH:$(pwd)
python3 -c "import suprafit; print(suprafit.__version__)"
```

## Schnellstart

### Einfaches Beispiel

```python
import suprafit as sf

# Daten laden
data = sf.io.load_data("experiment.txt")
print(f"Geladen: {data.DataPoints()} Datenpunkte")

# Modell erstellen und fitten
model = sf.models.create_model("nmr_1_1", data)
model.setGlobalParameter(0, 1000.0)  # K (Bindungskonstante)
result = sf.models.fit_model(model)

print(f"Fit erfolgreich: {result['success']}")
print(f"SSE: {result['sse']:.4f}")

# Ergebnisse exportieren
sf.io.export_results(model, "results.csv", "csv")
```

### Statistische Analyse

```python
import suprafit as sf

# Daten laden und Modell fitten
data = sf.io.load_data("experiment.txt")
model = sf.models.create_model("itc_1_1", data)
sf.models.fit_model(model)

# Monte Carlo Unsicherheitsanalyse
mc_result = sf.statistics.monte_carlo(
    model,
    iterations=10000,
    confidence=0.95
)
print(f"Monte Carlo konvergiert: {mc_result['converged']}")

# Cross-Validation
cv_result = sf.statistics.cross_validation(
    model,
    cv_type=1,  # Leave-one-out
    folds=5
)
print(f"CV Score: {cv_result['cv_score']:.4f}")

# Konfidenzintervalle (Perzentil-Methode)
ci_result = sf.statistics.confidence_intervals(
    model,
    iterations=10000,
    lower=0.025,  # 2.5. Perzentil
    upper=0.975   # 97.5. Perzentil
)

# Statistische Zusammenfassung
summary = sf.statistics.statistical_summary(model)
print(f"Modell: {summary['name']}")
print(f"Parameter: {summary['global_parameters']}")
```

### Modellvergleich

```python
import suprafit as sf

data = sf.io.load_data("experiment.txt")

# Mehrere Modelle erstellen und fitten
models = []
for model_type in ["nmr_1_1", "nmr_2_1"]:
    model = sf.models.create_model(model_type, data)
    sf.models.fit_model(model)
    models.append(model)

# Modelle vergleichen
comparison = sf.statistics.compare_models(models)
print("AIC-Werte:", comparison['aic'])

for i, stats in enumerate(comparison['models']):
    print(f"Modell {i+1}: {stats['name']}, SSE={stats['sse']:.4f}")
```

## Verfügbare Modelle

```python
import suprafit as sf
print(sf.models.available_models())
```

### Unterstützte Modelltypen

**NMR Titration:**
- `nmr_1_1` - 1:1 Bindung
- `nmr_2_1` - 2:1 Bindung
- `nmr_1_1_1_2` - 1:1 und 1:2 Bindung
- `nmr_2_1_1_1` - 2:1 und 1:1 Bindung

**ITC (Isotherme Titrationskalorimetrie):**
- `itc_1_1` - 1:1 Bindung
- `itc_1_2` - 1:2 Bindung
- `itc_2_1` - 2:1 Bindung
- `itc_2_2` - 2:2 Bindung

**Fluoreszenz:**
- `fl_1_1` - 1:1 Bindung
- `fl_1_1_1_2` - 1:1 und 1:2 Bindung

**UV-Vis Spektroskopie:**
- `uv_1_1` - 1:1 Bindung
- `uv_1_1_1_2` - 1:1 und 1:2 Bindung

## API-Referenz

### Modul-Struktur

```
suprafit/
├── io          # Ein-/Ausgabe
├── data        # Datenhandling
├── models      # Modellanpassung
└── statistics  # Statistische Analyse
```

### I/O Module (`sf.io`)

#### `load_data(filename, format="auto")`
Lädt Daten aus einer Datei.
- **Parameter:**
  - `filename`: Pfad zur Datei
  - `format`: "auto", "txt", "dat", "json", "itc", "csv"
- **Rückgabe:** `DataClass` Objekt

#### `save_data(data, filename, format="json")`
Speichert Daten in eine Datei.

#### `save_model(model, filename, include_statistics=True)`
Speichert Modell als JSON.

#### `export_results(model, filename, format="csv")`
Exportiert Ergebnisse (CSV oder TXT).

#### `supported_formats()`
Gibt unterstützte Dateiformate zurück.

### Data Module (`sf.data`)

#### `DataClass`
Haupt-Datencontainer.

**Methoden:**
- `Size()` - Anzahl Datenpunkte
- `DataPoints()` - Anzahl Datenpunkte
- `SeriesCount()` - Anzahl Serien
- `IndependentModel()` - Unabhängige Variablen
- `DependentModel()` - Abhängige Variablen
- `setDataBegin(int)` - Datenbereich Start
- `setDataEnd(int)` - Datenbereich Ende
- `UUID()` - Eindeutige ID
- `ProjectTitle()` - Projekttitel

#### `DataTable`
Datenmatrix (Eigen-basiert).

**Methoden:**
- `rowCount()` - Anzahl Zeilen
- `columnCount()` - Anzahl Spalten
- `data(row, col)` - Wert an Position
- `setData(row, col, value)` - Wert setzen
- `toList()` - Als Python-Liste

### Models Module (`sf.models`)

#### `create_model(type, data)`
Erstellt ein Modell.
- **Parameter:**
  - `type`: Modelltyp (z.B. "nmr_1_1")
  - `data`: DataClass Objekt
- **Rückgabe:** AbstractModel Instanz

#### `fit_model(model)`
Fittet ein Modell an die Daten.
- **Rückgabe:** Dictionary mit:
  - `success`: bool
  - `sse`: Sum of Squared Errors
  - `sey`: Standard Error

#### `available_models()`
Liste verfügbarer Modelltypen.

### Statistics Module (`sf.statistics`)

#### `monte_carlo(model, iterations=10000, confidence=0.95)`
Monte Carlo Unsicherheitsanalyse.

#### `cross_validation(model, cv_type=1, folds=5)`
Cross-Validation.
- `cv_type=1`: Leave-one-out

#### `calculate_aic(models)`
AIC-Vergleich für mehrere Modelle.

#### `compare_models(models)`
Multi-Kriterien-Vergleich.

#### `confidence_intervals(model, iterations=10000, lower=0.025, upper=0.975)`
Konfidenzintervalle (Perzentil-Methode).

#### `statistical_summary(model)`
Umfassende statistische Zusammenfassung.

## Beispiele

Vollständige Beispiele finden Sie in:
- `examples/python/basic_example.py` - Grundlegende Verwendung
- `examples/python/advanced_statistics.py` - Erweiterte statistische Analyse
- `examples/python/test_import.py` - Import-Test

## Integration in Workflows

### Jupyter Notebooks

```python
import suprafit as sf
import matplotlib.pyplot as plt
import numpy as np

# Daten laden und plotten
data = sf.io.load_data("experiment.txt")
indep = np.array(data.IndependentModel().toList())
dep = np.array(data.DependentModel().toList())

plt.figure(figsize=(10, 6))
plt.scatter(indep, dep, label='Experimental')
plt.xlabel('X')
plt.ylabel('Y')
plt.legend()
plt.show()
```

### Batch-Verarbeitung

```python
import suprafit as sf
import glob

results = []
for filename in glob.glob("experiments/*.txt"):
    data = sf.io.load_data(filename)
    model = sf.models.create_model("nmr_1_1", data)
    result = sf.models.fit_model(model)

    if result['success']:
        results.append({
            'file': filename,
            'sse': result['sse'],
            'K': model.GlobalParameter(0)
        })

# Ergebnisse analysieren
import pandas as pd
df = pd.DataFrame(results)
print(df)
```

## Fehlerbehandlung

```python
import suprafit as sf

try:
    data = sf.io.load_data("nonexistent.txt")
except Exception as e:
    print(f"Fehler beim Laden: {e}")

try:
    model = sf.models.create_model("invalid_type", data)
except Exception as e:
    print(f"Ungültiger Modelltyp: {e}")
```

## Performance-Hinweise

1. **Monte Carlo Iterationen**: Starten Sie mit 1000-5000 für Tests, verwenden Sie 10000+ für Produktion
2. **Datenbereich**: Verwenden Sie `setDataBegin()`/`setDataEnd()` um nur relevante Daten zu fitten
3. **Speicherverwaltung**: Python verwaltet den Speicher automatisch via pybind11

## Bekannte Einschränkungen

- Qt-Signale/Slots sind derzeit nicht in Python verfügbar
- Große Datenmengen (>100k Punkte) können langsam sein
- Threading ist durch Python GIL eingeschränkt

## Technische Details

### Implementierung
- Basiert auf **pybind11** (modern, header-only)
- Automatische Typ-Konvertierung zwischen Python und C++
- Qt QString ↔ Python str Konvertierung
- Eigen Matrix ↔ Python list Konvertierung
- QJsonObject als JSON-String in Python

### Memory Management
- Qt-Parent/Child-Ownership wird respektiert
- Python Garbage Collection für Python-Objekte
- Keine manuellen Speicherfreigaben nötig

## Lizenz

GNU General Public License v3.0

Copyright (C) 2025 Conrad Hübler <Conrad.Huebler@gmx.net>

Python-Schnittstelle erstellt von Claude Code AI Assistant.

## Support

Bei Problemen oder Fragen:
1. Überprüfen Sie die Beispiele in `examples/python/`
2. Führen Sie `test_import.py` aus, um die Installation zu verifizieren
3. Konsultieren Sie `src/python_bindings/README.md` für Build-Details

## Weiterführende Dokumentation

- SupraFit C++ Dokumentation: `docs/`
- pybind11 Dokumentation: https://pybind11.readthedocs.io/
- Python-Bindings Quellcode: `src/python_bindings/`
