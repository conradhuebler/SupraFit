"""Locate the `suprafit_cli` executable and run it on a task-config JSON.

Discovery mirrors src/tests/test_utils.h: the SUPRAFIT_CLI_PATH env var first, then the common
build output directories, then PATH. Claude Generated.
"""

from __future__ import annotations

import os
import shutil
import subprocess
from pathlib import Path

from .errors import CLIExecutionError, SupraFitNotFoundError

# Candidate directories (relative to the repo root = parent of this package's parent).
_REPO_ROOT = Path(__file__).resolve().parents[2]
_EXE_NAME = "suprafit_cli"


def _candidate_dirs() -> list[Path]:
    env = os.environ.get("SUPRAFIT_CLI_PATH")
    candidates: list[Path] = []
    if env:
        candidates.append(Path(env))
    # build/bin layouts used by the CMake presets
    candidates += [
        _REPO_ROOT / "debug" / "bin" / "linux",
        _REPO_ROOT / "release" / "bin" / "linux",
        _REPO_ROOT / "build-asan" / "bin" / "linux",
        _REPO_ROOT / "build" / "bin" / "linux",
    ]
    return candidates


def find_cli() -> Path:
    """Return the path to `suprafit_cli`, or raise SupraFitNotFoundError listing the search."""
    env = os.environ.get("SUPRAFIT_CLI_PATH")
    if env:
        p = Path(env)
        if p.is_file() and os.access(p, os.X_OK):
            return p
    for d in _candidate_dirs():
        cand = d / _EXE_NAME
        if cand.is_file() and os.access(cand, os.X_OK):
            return cand
    on_path = shutil.which(_EXE_NAME)
    if on_path:
        return Path(on_path)
    searched = [str(p) for p in _candidate_dirs()]
    if env:
        searched.insert(0, f"SUPRAFIT_CLI_PATH={env}")
    searched.append("PATH")
    raise SupraFitNotFoundError(
        f"could not find `{_EXE_NAME}`; set SUPRAFIT_CLI_PATH or build it. Searched: {searched}"
    )


def run_cli(task_config_path: Path, output_path: Path, nproc: int = 4, timeout: float | None = None) -> None:
    """Invoke `suprafit_cli -i <task_config> -o <output>`. Raises CLIExecutionError on failure."""
    cli = find_cli()
    cmd = [str(cli), "-i", str(task_config_path), "-o", str(output_path), "-n", str(nproc)]
    try:
        proc = subprocess.run(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            timeout=timeout,
            check=False,
        )
    except FileNotFoundError as e:
        raise SupraFitNotFoundError(f"suprafit_cli not found at {cli}: {e}") from e
    except subprocess.TimeoutExpired as e:
        raise CLIExecutionError(f"suprafit_cli timed out after {timeout}s", cmd=cmd) from e
    if proc.returncode != 0:
        stderr_tail = (proc.stderr or b"").decode("utf-8", "replace").strip().splitlines()[-20:]
        raise CLIExecutionError(
            f"suprafit_cli exited {proc.returncode}",
            cmd=cmd,
            returncode=proc.returncode,
            stderr="\n".join(stderr_tail),
        )