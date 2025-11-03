# Analysis of Raw Statistical Data Storage in SupraFit JSON

This document details how the raw data from statistical post-processing is stored within the SupraFit JSON structure.

## 1. Storage Mechanism

The raw data, which typically consists of a large number of sample values for each parameter (e.g., from a Monte Carlo simulation), is stored as a single string in the `data.raw` field of the JSON. The numerical values within the string are separated by spaces.

**Example JSON Snippet:**

```json
{
  "data": {
    "raw": "-274.303 -274.303 ..."
  },
  ...
}
```

## 2. Data Generation and Conversion

1.  **Data Generation:** The statistical methods in `analyse.cpp` generate a `QVector<qreal>` (or a similar container) of sample values for each parameter.
2.  **Conversion to String:** To store this data in the JSON, the `QVector<qreal>` is converted into a space-separated `QString` using the `ToolSet::DoubleVec2String` function.

## 3. Data Parsing and Processing

1.  **Parsing from String:** When the JSON file is read, the `data.raw` string is parsed back into a `QVector<qreal>` using the `ToolSet::String2DoubleVec` function.
2.  **Processing:** This vector of raw data is then used for various statistical calculations, such as:
    *   Creating histograms (`ToolSet::List2Histogram`)
    *   Calculating box-plot statistics (`ToolSet::BoxWhiskerPlot`)
    *   Determining confidence intervals (`ToolSet::Confidence`)
    *   Calculating entropy (`ToolSet::Entropy`)

## 4. Rationale and Trade-offs

*   **Rationale:**
    *   **Compactness:** Storing the data as a single string can be more compact than a JSON array, reducing file size.
    *   **Simplicity:** The implementation of the string-based storage is relatively straightforward.
*   **Trade-offs:**
    *   **Precision:** There is a potential for a minor loss of precision when converting floating-point numbers to strings and back.
    *   **Overhead:** String parsing introduces a small amount of overhead compared to directly reading a JSON array.

## 5. Conclusion

The storage of raw statistical data as a space-separated string is a deliberate design choice in SupraFit to balance file size and implementation simplicity. The `ToolSet` library provides the necessary functions for this conversion, ensuring that the data can be efficiently stored and retrieved for analysis.
