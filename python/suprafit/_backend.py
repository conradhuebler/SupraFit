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
    """Phase 2 stub: in-process pybind11 execution. Not implemented yet — raises if used so the
    CLI backend stays the only working path until the `suprafit._core` module exists."""

    def __init__(self, *args, **kwargs):
        try:
            import suprafit._core  # noqa: F401  (Phase 2)
        except ImportError as e:
            raise NotImplementedError(
                "NativeBackend (pybind11 suprafit._core) is not built yet — use the CLI backend. "
                "See roadmap/python_interface.md Phase 2."
            ) from e

    def run(self, task_config: dict, nproc: int = 4, timeout: float | None = None) -> dict:
        raise NotImplementedError("NativeBackend.run() is Phase 2; use CLIBackend.")


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