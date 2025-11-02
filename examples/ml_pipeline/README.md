# ML Pipeline Example

This directory contains a correct, single-file example of a machine learning pipeline using `suprafit_cli`.

This configuration is based on the working example `input/Test_AddModels_v2.json`.

## Workflow

The entire pipeline is defined in a single JSON file and executed with a single command. It performs the following steps:

1.  **Generate Independent Data**: Generates 20 NMR titration data points with realistic concentrations:
    - **Host (receptor)**: Constant at **1 mM** (realistic for NMR)
    - **Guest (ligand)**: Increases from **0 to 1.9 equivalents** (typical titration range)
    - Equations: `"0.001|(X - 1) * 0.0001"` generates proper binding isotherms
2.  **Generate Dependent Data**: Generates the corresponding dependent data (chemical shift changes) based on a "true" theoretical model (Model ID 1), with Gaussian noise to simulate experimental data.
3.  **Repeat**: Repeats the generation 1 time (default). Configure with `"Repeat": N` in the `Main` section to generate N datasets.
4.  **Fit Models**: Fits four different theoretical models (IDs 1, 2, 3, and 4) to each dataset.
5.  **Analyze**: Performs post-fit statistical analysis (Monte Carlo with 1000 steps, optional Cross-Validation) for each fit.

## Files

*   `ml_pipeline_example.json`: The single configuration file for the entire workflow.

## How to Run

Execute the following command from the root directory of the project (`/home/conrad/src/SupraFit`).

```bash
./build/release/bin/linux/suprafit_cli -i examples/ml_pipeline/ml_pipeline_example.json
```

This command will:

*   Read the configuration file.
*   Execute the complete workflow described above.
*   Save the results into three files with the prefix `ml_pipeline_example.json-0`:
    - `-0.json`: Multi-project dataset with all fitted models and data (4 projects for 4 models)
    - `-models-0.json`: Project file containing all fitted models (SupraFit format)
    - `-0-ml.json`: ML-optimized dataset with statistical features extracted for machine learning
    - `-0suprafit`: Compressed project file in SupraFit binary format