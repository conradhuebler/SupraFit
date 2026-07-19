"""The public Project API: build/load a project, add models, fit, run post-processing, read results.

`Project` builds a task-config dict, hands it to a Backend (CLI by default), and parses the output
JSON into `Model` objects. NumPy is optional; without it, table results fall back to lists.

Claude Generated.
"""

from __future__ import annotations

import json
from pathlib import Path
from typing import Iterable

from . import _backend, _config, _data, _jobs, _models, _results


class Project:
    """A SupraFit project: data + models + post-processing, driven through a Backend."""

    def __init__(self, *, independent: dict, dependent: dict, data_sources: _data.DataSources | None = None,
                 backend: _backend.Backend | None = None, outfile: str = "suprafit_result",
                 system_parameters: dict | None = None):
        self._independent = independent
        self._dependent = dependent
        self._data_sources = data_sources
        self._backend = backend or _backend.get_backend()
        self._outfile = outfile
        # ITC etc. system parameters (cell/syringe concentration, cell volume, temperature),
        # resolved to {int index: float value}; only the native backend consumes them. Claude Generated.
        from . import _native
        self._system_parameters = _native.resolve_system_parameters(system_parameters)
        # pending models: label -> {id, options, definition}
        self._models: dict[str, dict] = {}
        # pending post-fit methods (controller dicts)
        self._post_methods: list[dict] = []
        # results after fit()/run()
        self.models: list[_results.Model] = []

    # ---- constructors --------------------------------------------------
    @classmethod
    def from_arrays(cls, indep, dep, **kwargs) -> "Project":
        """Build a project from numpy/list arrays (indep: rows x n_vars, dep: rows x n_series)."""
        ds = _data.DataSources()
        indep_block, dep_block = ds.from_arrays(indep, dep)
        return cls(independent=indep_block, dependent=dep_block, data_sources=ds, **kwargs)

    @classmethod
    def from_file(cls, path, *, indep_cols: slice, dep_cols: slice, rows: int | None = None, **kwargs) -> "Project":
        """Slice an existing whitespace-delimited data file into Independent/Dependent blocks."""
        indep_block, dep_block = _data.DataSources.from_file(path, indep_cols=indep_cols, dep_cols=dep_cols, rows=rows)
        return cls(independent=indep_block, dependent=dep_block, **kwargs)

    @classmethod
    def from_equations(cls, equations: str, n_points: int, n_vars: int, **kwargs) -> "Project":
        """Build the independent table from the CLI's equation generator."""
        return cls(independent=_data.DataSources.from_equations(equations, n_points, n_vars), dependent={}, **kwargs)

    # ---- model + post-processing configuration -------------------------
    def add_model(self, name_or_id, *, options: dict | None = None, definition: dict | None = None,
                  label: str | None = None) -> "Project":
        """Queue a model for the next fit. `name_or_id` is a snake_case name or integer id."""
        mid = _models.model_id(name_or_id)
        mname = _models.model_name(mid)
        lbl = label or mname
        # disambiguate duplicate labels so AddModels keys stay unique
        if lbl in self._models:
            i = 2
            while f"{lbl}_{i}" in self._models:
                i += 1
            lbl = f"{lbl}_{i}"
        self._models[lbl] = {"id": mid, "options": options, "definition": definition}
        return self

    def add_post_processing(self, controller: dict) -> "Project":
        """Queue a raw post-processing controller (from _jobs)."""
        self._post_methods.append(dict(controller))
        return self

    def monte_carlo(self, **kwargs) -> "Project":
        return self.add_post_processing(_jobs.monte_carlo(**kwargs))

    def cross_validation(self, **kwargs) -> "Project":
        return self.add_post_processing(_jobs.cross_validation(**kwargs))

    def reduction(self, **kwargs) -> "Project":
        return self.add_post_processing(_jobs.reduction(**kwargs))

    def weakened_grid_search(self, **kwargs) -> "Project":
        return self.add_post_processing(_jobs.weakened_grid_search(**kwargs))

    def model_comparison(self, **kwargs) -> "Project":
        return self.add_post_processing(_jobs.model_comparison(**kwargs))

    def fast_confidence(self, **kwargs) -> "Project":
        return self.add_post_processing(_jobs.fast_confidence(**kwargs))

    def global_search(self, **kwargs) -> "Project":
        return self.add_post_processing(_jobs.global_search(**kwargs))

    # ---- execution -----------------------------------------------------
    def _task_config(self) -> dict:
        return _config.build_task_config(
            independent=self._independent,
            dependent=self._dependent,
            models=self._models,
            post_fit_methods=self._post_methods,
            out_file=self._outfile,
            system_parameters=self._system_parameters,
        )

    def fit(self, nproc: int = 4, timeout: float | None = None) -> list[_results.Model]:
        """Execute the queued models (+ post-processing) and parse the results."""
        if not self._models:
            raise ValueError("no models added; call add_model(...) before fit()")
        config = self._task_config()
        output = self._backend.run(config, nproc=nproc, timeout=timeout)
        self.models = _results.parse_project(output)
        return self.models

    # ---- access --------------------------------------------------------
    def model(self, name_or_id) -> _results.Model | None:
        """Return the first fitted model whose id matches `name_or_id` (name or int)."""
        mid = _models.model_id(name_or_id)
        for m in self.models:
            if m.model_id == mid:
                return m
        return None

    def results_frame(self):
        """Return a pandas DataFrame with one row per fitted model (from `Model.features()`).

        Requires pandas (optional). Raises ImportError with guidance if it is not installed.
        Claude Generated."""
        try:
            import pandas as pd
        except ImportError as e:
            raise ImportError(
                "results_frame() needs pandas (pip install pandas); without it use "
                "[m.features() for m in project.models]."
            ) from e
        return pd.DataFrame([m.features() for m in self.models])

    def save_config(self, path) -> None:
        """Write the current task-config to disk (useful for debugging / CLI re-runs)."""
        Path(path).write_text(json.dumps(self._task_config(), indent=2))

    def cleanup(self) -> None:
        if self._data_sources is not None:
            self._data_sources.cleanup()