"""Backend seam — how a task-config dict actually gets executed.

`CLIBackend` shells out to `suprafit_cli` (Phase 1). `NativeBackend` is a stub for the Phase 2
pybind11 in-process variant; scripts written against the CLI backend keep working when the native
backend lands. Claude Generated.
"""

from __future__ import annotations

import abc
import json
import tempfile
import zlib
from pathlib import Path

from . import _cli
from .errors import CLIExecutionError, ResultParseError


def _decompress_suprafit(path: Path) -> dict:
    """Read a `.suprafit` file: qCompress'd JSON (zlib stream with a 4-byte length header),
    or plain JSON. Returns the parsed dict. Claude Generated."""
    raw = path.read_bytes()
    if not raw:
        raise ResultParseError(f"output file is empty: {path}")
    # qCompress prepends a 4-byte big-endian length; zlib.decompress skips it if we offset by 4.
    for offset, label in ((4, "qCompress"), (0, "plain-zlib")):
        try:
            return json.loads(zlib.decompress(raw[offset:]))
        except (zlib.error, json.JSONDecodeError, UnicodeDecodeError):
            continue
    try:
        return json.loads(raw.decode("utf-8"))
    except (json.JSONDecodeError, UnicodeDecodeError) as e:
        raise ResultParseError(f"could not parse {path} as (compressed) JSON: {e}") from e


def _table_from_block(block, np):
    """Reconstruct a 2D float array from a task-config Independent/Dependent block.

    Only the file source (`Source: "file"`, written by `Project.from_arrays` / `from_file`) is
    supported by the native backend; the equation/model generators are CLI-only. Claude Generated."""
    src = block.get("Source") if isinstance(block, dict) else None
    if src != "file":
        raise NotImplementedError(
            "NativeBackend supports only array/file data (Source: 'file'); the equation/model "
            f"generators are CLI-only. Got Source={src!r}. Use the CLI backend for generated data."
        )
    spec = block.get("File", {})
    arr = np.loadtxt(spec["Path"])
    if arr.ndim == 1:
        arr = arr.reshape(-1, 1)
    start_row, start_col = int(spec.get("StartRow", 0)), int(spec.get("StartCol", 0))
    rows, cols = int(spec.get("Rows", 0)), int(spec.get("Cols", 0))
    end_row = start_row + rows if rows > 0 else arr.shape[0]
    end_col = start_col + cols if cols > 0 else arr.shape[1]
    return np.ascontiguousarray(arr[start_row:end_row, start_col:end_col], dtype=float)


def _find_fitted_project(td: Path, base: str) -> Path:
    """Locate the fitted project file the CLI writes: `<base>-project-0.suprafit` (ML-pipeline path).
    Falls back to any `*-project-*.suprafit`, then any `*.suprafit` in the temp dir."""
    candidates = sorted(td.glob(f"{base}-project-*.suprafit"))
    if candidates:
        return candidates[0]
    candidates = sorted(td.glob("*-project-*.suprafit"))
    if candidates:
        return candidates[0]
    supra = sorted(td.glob("*.suprafit"))
    if supra:
        return supra[0]
    raise ResultParseError(
        f"no fitted .suprafit project found in {td}; files: {[p.name for p in sorted(td.iterdir())]}"
    )


class Backend(abc.ABC):
    """Abstract executor: run a task-config, return the parsed output project JSON dict."""

    @abc.abstractmethod
    def run(self, task_config: dict, nproc: int = 4, timeout: float | None = None) -> dict:
        """Execute `task_config` and return the output project JSON as a dict."""


class CLIBackend(Backend):
    """Executes via `suprafit_cli -i cfg.json -o out.json` in a temp directory."""

    def __init__(self, keep_temps: bool = False, tmpdir: str | Path | None = None):
        self.keep_temps = keep_temps
        self.tmpdir = str(tmpdir) if tmpdir else None

    def run(self, task_config: dict, nproc: int = 4, timeout: float | None = None) -> dict:
        with tempfile.TemporaryDirectory(prefix="suprafit_", dir=self.tmpdir, delete=not self.keep_temps) as td:
            td_path = Path(td)
            cfg = td_path / "task.json"
            # The CLI writes `<base>-project-0.suprafit` next to the `-o` path; use a fixed base here
            # so we can locate it deterministically. Claude Generated.
            out = td_path / "result"
            cfg.write_text(json.dumps(task_config))
            try:
                _cli.run_cli(cfg, out, nproc=nproc, timeout=timeout)
            except CLIExecutionError:
                raise
            fitted = _find_fitted_project(td_path, "result")
            return _decompress_suprafit(fitted)


class NativeBackend(Backend):
    """In-process execution via the pybind11 `suprafit._core` module (Phase 2).

    Consumes the same task-config as the CLI backend and returns the same project JSON, but fits
    in-process — no subprocess, temp files, or file round-trip. Data must come from arrays or a data
    file (`Source: "file"`, i.e. `Project.from_arrays` / `from_file`); the equation/model generators
    (`Source: "generator"`) remain CLI-only. `nproc`/`timeout` are ignored (the fit path is
    synchronous; post-processing uses SupraFit's own QThreadPool). Claude Generated."""

    def __init__(self, *args, **kwargs):
        try:
            import suprafit._core as _core
        except ImportError as e:
            raise NotImplementedError(
                "NativeBackend needs the pybind11 module suprafit._core, which is not built. "
                "Build it with `cmake -DSUPRAFIT_PYBIND=ON` (see roadmap/python_interface.md Phase 2)."
            ) from e
        self._core = _core

    def run(self, task_config: dict, nproc: int = 4, timeout: float | None = None) -> dict:
        try:
            import numpy as np
        except ImportError as e:
            raise NotImplementedError("NativeBackend requires numpy for in-process data exchange.") from e
        indep = _table_from_block(task_config.get("Independent", {}), np)
        dep = _table_from_block(task_config.get("Dependent", {}), np)
        models_json = json.dumps(task_config.get("AddModels", {}))
        analysis_json = json.dumps(task_config.get("PostFitAnalysis", {}) or {})
        project_json = self._core.fit_from_tables(indep, dep, models_json, analysis_json, int(nproc))
        try:
            return json.loads(project_json)
        except (json.JSONDecodeError, TypeError) as e:
            raise ResultParseError(f"native backend returned unparseable JSON: {e}") from e


_BACKENDS = {"cli": CLIBackend, "native": NativeBackend}
_default: Backend = CLIBackend()


def get_backend() -> Backend:
    return _default


def set_backend(name: str, **kwargs) -> Backend:
    """Switch the process-wide default backend. `name` is 'cli' (default) or 'native' (Phase 2)."""
    global _default
    if name not in _BACKENDS:
        raise ValueError(f"unknown backend {name!r}; choose from {list(_BACKENDS)}")
    _default = _BACKENDS[name](**kwargs)
    return _default