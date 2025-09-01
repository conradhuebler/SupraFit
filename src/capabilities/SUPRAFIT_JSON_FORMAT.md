# SupraFit JSON/suprafit Format Dokumentation

**Erstellt:** 2025-08-31  
**Autor:** Claude (basierend auf Codebase-Analyse)  
**Zweck:** Vollständige Strukturdokumentation für ML-Feature-Extraktion

## 📁 Dateitypen

- **.suprafit**: Komprimierte JSON-Struktur (via `JsonHandler::LoadFile()` lesbar)
- **.json**: Unkomprimierte JSON-Struktur  
- **Struktur**: Identisch, nur Komprimierung unterschiedlich

---

## 🏗️ Root-Level Struktur Varianten

### **Typ A: Standard SupraFit Project (.suprafit, ML-Pipeline)**
```json
{
  "data": {
    "DataType": 1,
    "SupraFit": 2004,
    "content": "Generated with DataGenerator...\n\nInput Configuration:\n{\"Dependent\":{\"Noise\":{\"RandomSeed\":12345,\"Std\":[0.001,0.001],\"Type\":\"gaussian\"},...}}",
    "dependent": { "data": {"0": "6.780 2.464", ...}, "cols": 2 },
    "independent": { "data": {"0": "0.001 0.000", ...}, "cols": 2 },
    "title": "", "uuid": "{...}", "timestamp": 1756669705366
  },
  "model_0": {
    "fit_quality": {...},
    "model_id": 1, "model_name": "nmr_1_1",
    "post_fit_analysis": {
      "analysis_completed": true,
      "methods": {
        "1": {
          "method": 1, "method_name": "Monte Carlo Simulation",
          "configuration": {"EntropyBins": 50, "MaxSteps": 1000, "Method": 1, "VarianceSource": 2},
          "results": {        // ⭐ HIER SIND DIE ECHTEN DATEN!
            "0": { /* Parameter 0 Analyse */ },
            "1": { /* Parameter 1 Analyse */ }
          }
        }
      }
    }
  },
  "model_1": { /* Zweites Modell */ }
}
```

### **Typ B: Direct Analysis (vonHand_mc.json)**
```json
{
  "data": { /* Standard Datenstruktur */ },
  "methods": {           // ⭐ Post-Processing DIREKT im Root!
    "0": {},             // Leer (keine Analyse)
    "1": {               // Method ID 1 = Monte Carlo
      "0": { /* Parameter 0 direkt */ },
      "1": { /* Parameter 1 direkt */ }
    }
  }
}
```

### **Typ C: ML-Pipeline (test_addmodels_v2-X.json)**
```json
{
  "data": {
    "raw": {
      "ml_pipeline": {
        "generation_config": {
          "ground_truth_model": {
            "id": 1, "name": "¹H 1:1-Model",
            "global_parameters": [2.33], "local_parameters": [[6.81, 2.41], [6.07, 2.28]]
          },
          "input_json": { /* Original-Konfiguration */ }
        },
        "fitted_models": [
          {
            "model_id": 1, "model_name": "nmr_1_1", "fit_quality": {...},
            "post_fit_analysis": {
              "methods": {
                "1": { /* Monte Carlo Daten */ }
              }
            }
          }
        ]
      }
    }
  }
}
```

---

## 🎯 Parameter-Analyse Struktur (Kernstück!)

**Korrekte Pfade für statistische Daten:**

### **Pfad A (Standard .suprafit/.json):**
`root.model_X.post_fit_analysis.methods.METHOD_ID.results.PARAM_INDEX`

### **Pfad B (Direct Analysis):**
`root.methods.METHOD_ID.PARAM_INDEX`

### **Pfad C (ML-Pipeline):**
`root.data.raw.ml_pipeline.fitted_models[X].post_fit_analysis.methods.METHOD_ID.results.PARAM_INDEX`

### **Parameter-Struktur (alle Pfade identisch):**
```json
{
  "boxplot": {
    "count": 2000,                    // Anzahl Monte Carlo Samples
    "mean": -89.178,                  // Mittelwert
    "median": -210.397,               // Median
    "stddev": 165.983,                // Standardabweichung
    "lower_quantile": -254.944,       // 25% Quantil
    "upper_quantile": 49.189,         // 75% Quantil  
    "lower_whisker": -274.303,        // Unterer Whisker
    "upper_whisker": 206.88,          // Oberer Whisker
    "extreme_outliers": "",           // Extreme Ausreißer (String)
    "mild_outliers": ""               // Milde Ausreißer (String)
  },
  "confidence": {
    "error": 95,                      // Konfidenz-Level (95%)
    "lower": -274.303,                // Untere Konfidenzgrenze
    "upper": 203.24                   // Obere Konfidenzgrenze
  },
  "data": {
    "raw": "-274.303 -274.303 ..."    // ⭐ ROHDATEN String (Space-separated)
  },
  "index": "0",                       // Parameter-Index als String
  "name": "lg K₁₁",                   // Parameter-Name
  "type": "Global Parameter",         // Parameter-Typ (Global/Local Parameter)
  "value": 4.45926,                   // Optimal-Wert (fitted value)
  "x": "X1 X2 X3 ...",               // Histogramm X-Koordinaten (String)
  "y": "Y1 Y2 Y3 ..."                // Histogramm Y-Koordinaten (String)
}
```

---

## 📊 Method IDs

```cpp
// src/core/toolset.cpp:1108 & SupraFit enums
1  = Monte Carlo Simulation
2  = Cross Validation  
3  = Model Comparison
4  = Weakened Grid Search
5  = Fast Confidence
6  = Reduction Analysis
7  = Global Search
```

---

## 🧬 Noise-Konfiguration Extraktion

**Location:** `root.data.content` (String mit eingebettetem JSON)

**Format:**
```
Generated with DataGenerator version 1
Model: ¹H 1:1-Model (parameters)
...

Input Configuration:
{"Dependent":{"Generator":{...},"Noise":{"RandomSeed":12345,"Std":[0.001,0.001],"Type":"gaussian"},"Source":"generator"},"Independent":{...},"Main":{...}}
```

**Parsing-Strategie:**
1. `content` String extrahieren
2. Letztes JSON-Objekt `{...}` finden (RegEx)
3. JSON parsen: `QJsonDocument::fromJson()`
4. Pfad: `parsed.Dependent.Noise`

**Noise-Struktur:**
```json
{
  "RandomSeed": 12345,
  "Std": [0.001, 0.001],    // Array für multiple Series
  "Type": "gaussian"
}
```

---

## 🔍 TextFromConfidence Funktion (Referenz)

**Location:** `src/core/toolset.cpp:1106`

**Zeigt korrekte Datenverarbeitung:**
```cpp
qreal value = result["value"].toDouble();           // Optimal-Wert
QJsonObject confidence = result["confidence"].toObject();
qreal upper = confidence["upper"].toDouble();       // Obere Grenze  
qreal lower = confidence["lower"].toDouble();       // Untere Grenze
qreal conf = confidence["error"].toDouble();        // 95% Konfidenz

QString name = result["name"].toString();           // "lg K₁₁"
QString type = result["type"].toString();           // "Global Parameter"
```

---

## ⚙️ fit_quality Struktur (alle Modelle)

```json
"fit_quality": {
  "aic": -101.498,                    // Akaike Information Criterion
  "aicc": -95.036,                    // Corrected AIC
  "aic_aicc_ratio": 1,                // AIC/AICc Verhältnis
  "chi_squared": 0.0449,              // Chi²-Statistik
  "data_points": 20,                  // Anzahl Datenpunkte
  "data_to_param_ratio": 6.667,       // Datenpunkte/Parameter Verhältnis
  "global_params": 1,                 // Anzahl globale Parameter
  "local_params": 2,                  // Anzahl lokale Parameter  
  "parameter_count": 3,               // Gesamtparameter
  "r_squared": 0.9996,                // R² Bestimmtheitsmaß
  "rmse": 0.0601,                     // Root Mean Square Error
  "series_count": 2,                  // Anzahl Datenreihen
  "sse": 0.0686,                      // Sum of Squared Errors
  "sse_per_point": 0.00343,           // SSE pro Datenpunkt
  "log_sse": -1.164                   // Logarithmus von SSE
}
```

---

## 🚀 Implementierungs-Hinweise

### **Korrekte Feature-Extraktion:**

1. **Strukturerkennung:**
   ```cpp
   if (root.contains("methods")) {
       // Typ B: Direct Analysis
       parseMethods(root["methods"]);
   } else if (root.contains("model_0")) {
       // Typ A: Standard SupraFit
       parseModels(root);
   } else if (root.contains("data") && root["data"].contains("raw")) {
       // Typ C: ML-Pipeline
       parseMLPipeline(root["data"]["raw"]);
   }
   ```

2. **ToolSet Funktionen verwenden:**
   ```cpp
   QVector<double> values = ToolSet::String2DoubleVec(data["raw"].toString());
   // Dann: std::accumulate, std::inner_product für Statistiken
   ```

3. **Noise-Parsing:**
   ```cpp
   QString content = data["content"].toString();
   // RegEx: QRegularExpression(".*\\n.*\\{.*\\}$")
   // JSON extrahieren und Dependent.Noise finden
   ```

4. **Robuste Parameter-Extraktion:**
   ```cpp
   // Alle verfügbaren Parameter durchlaufen (0, 1, 2, ...)
   // boxplot, confidence, data.raw extrahieren
   // mean, stddev, quantiles berechnen
   ```

---

## ✅ Status

- **Analysiert:** ✅ vonHand_mc.json, test_addmodels_v2-models-0.suprafit, test_addmodels_v2-X.json
- **Implementierung:** ❌ MLFeatureExtractor muss korrigiert werden
- **TextFromConfidence:** ✅ Referenz-Implementation verstanden