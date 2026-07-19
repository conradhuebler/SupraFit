"""Build the top-level task-config dict consumed by `suprafit_cli`'s `executeTaskConfiguration`.

Shape confirmed from test_simple.json, input/test_cli_nmr_4models.json, and
src/tests/test_post_processing.cpp::createPostProcessingConfig. Claude Generated.
"""

from __future__ import annotations

from typing import Iterable

from ._models import model_id, model_name


def add_models_block(models: dict) -> dict:
    """Serialise the Project's pending models into the CLI `AddModels` block.

    Each entry is `{name: {"ID": <id>, "Options": {...}}}` (the v2.0 form the CLI consumes at
    suprafit_cli.cpp:2236). The dict key is a log label; dispatch is by ID."""
    block: dict[str, dict] = {}
    for label, spec in models.items():
        mid = spec["id"]
        entry = {"ID": mid}
        opts = spec.get("options") or {}
        if opts:
            entry["Options"] = dict(opts)
        definition = spec.get("definition")
        if definition:
            entry["ModelDefinition"] = dict(definition)
        block[label] = entry
    return block


def build_task_config(
    *,
    independent: dict,
    dependent: dict,
    models: dict | None = None,
    post_fit_methods: Iterable[dict] | None = None,
    out_file: str = "suprafit_result",
    fit_models: bool = True,
    process_ml_pipeline: bool = False,
    noise: dict | None = None,
    system_parameters: dict | None = None,
) -> dict:
    """Assemble the full task-config dict."""
    config: dict = {
        "Main": {
            "OutFile": out_file,
            "UseModularStructure": True,
            "FitModels": bool(fit_models),
        },
        "Independent": independent,
        "Dependent": dependent,
    }
    # ITC (and other) system parameters as {index: value}; consumed in-process by the native
    # backend (the CLI backend ignores them for now). Claude Generated.
    if system_parameters:
        config["SystemParameters"] = {str(k): float(v) for k, v in system_parameters.items()}
    # The fit actually runs on the CLI's ML-pipeline path, which writes the fitted project to
    # `<base>-project-0.suprafit`; without ProcessMLPipeline the CLI only generates data. So force
    # it on whenever we are fitting (or running post-processing). Claude Generated.
    if process_ml_pipeline or fit_models:
        config["Main"]["ProcessMLPipeline"] = True
    if post_fit_methods:
        config["Main"]["PostFitAnalysis"] = True
        config["PostFitAnalysis"] = {"methods": list(post_fit_methods)}
    if noise:
        config["Dependent"].setdefault("Noise", {}).update(noise) if isinstance(config["Dependent"], dict) else None
    if models:
        config["AddModels"] = add_models_block(models)
    return config