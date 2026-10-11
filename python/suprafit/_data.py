"""Independent/Dependent data builders for the task-config JSON.

Three sources, matching what `suprafit_cli` consumes:
- arrays (numpy / list-of-lists) -> written to a temp CSV/.dat the CLI reads via `Source:"file"`
- a slice of an existing .dat file (`File` block with StartRow/StartCol/Rows/Cols)
- the CLI's equation generator (`Source:"generator"`, `Generator.Type:"equations"`)

For the array source we write a whitespace-delimited file (German or English decimals both accepted
by the CLI's file loader) and reference it by path. Claude Generated.
"""

from __future__ import annotations

import os
import tempfile
from pathlib import Path
from typing import Iterable

try:
    import numpy as _np  # noqa: F401
    _HAVE_NUMPY = True
except ImportError:
    _HAVE_NUMPY = False


def _to_list_rows(arr) -> list[list[float]]:
    """Coerce a numpy array or list-of-lists to a plain list of float rows."""
    if _HAVE_NUMPY and isinstance(arr, _np.ndarray):
        return arr.tolist()
    if isinstance(arr, (list, tuple)):
        rows = []
        for r in arr:
            if isinstance(r, (list, tuple)):
                rows.append([float(x) for x in r])
            else:
                rows.append([float(r)])
        return rows
    raise TypeError(f"expected an array-like (numpy.ndarray or list of lists), got {type(arr).__name__}")


def _write_dat(rows: list[list[float]], tmpdir: Path, name: str) -> Path:
    """Write rows to a whitespace-delimited file; returns its path."""
    p = tmpdir / name
    with p.open("w") as f:
        for row in rows:
            f.write(" ".join(repr(float(x)) for x in row))
            f.write("\n")
    return p


class DataSources:
    """Holds the Independent + Dependent blocks and any temp files they reference."""

    def __init__(self, tmpdir: Path | None = None):
        self._tmpdir = tmpdir
        self._files: list[Path] = []
        self._keep = tmpdir is not None

    # ---- array sources -------------------------------------------------
    def from_arrays(self, indep, dep, indep_file: str = "indep.dat", dep_file: str = "dep.dat") -> tuple[dict, dict]:
        """Build Independent/Dependent blocks from numpy/list arrays.

        `indep` is (rows x n_indep_vars), `dep` is (rows x n_series). Both must share the row count.
        """
        if self._tmpdir is None:
            ctx = tempfile.TemporaryDirectory(prefix="suprafit_data_", delete=not self._keep)
            self._tmpdir = Path(ctx.name)
            self._ctx = ctx  # keep alive
        indep_rows = _to_list_rows(indep)
        dep_rows = _to_list_rows(dep)
        if len(indep_rows) != len(dep_rows):
            raise ValueError(f"independent/dependent row mismatch: {len(indep_rows)} vs {len(dep_rows)}")
        indep_path = _write_dat(indep_rows, self._tmpdir, indep_file)
        dep_path = _write_dat(dep_rows, self._tmpdir, dep_file)
        self._files += [indep_path, dep_path]
        n_indep = len(indep_rows[0]) if indep_rows else 0
        n_dep = len(dep_rows[0]) if dep_rows else 0
        rows = len(indep_rows)
        indep_block = {
            "Source": "file",
            "File": {"Path": str(indep_path), "StartRow": 0, "StartCol": 0, "Rows": rows, "Cols": n_indep},
        }
        dep_block = {
            "Source": "file",
            "File": {"Path": str(dep_path), "StartRow": 0, "StartCol": 0, "Rows": rows, "Cols": n_dep},
        }
        return indep_block, dep_block

    # ---- file slice source --------------------------------------------
    @staticmethod
    def from_file(path: str | Path, *, indep_cols: slice, dep_cols: slice, rows: int | None = None) -> tuple[dict, dict]:
        """Slice an existing whitespace-delimited data file into Independent/Dependent blocks.

        `indep_cols`/`dep_cols` are Python slices over column indices (0-based). `rows` limits
        the row count (None = all).
        """
        def _slice_to_block(s: slice, path_str: str, name: str) -> dict:
            start, stop, step = s.indices(10**9)  # stop is symbolic; we emit StartCol + Cols only
            if step != 1:
                raise ValueError(f"{name}: only contiguous column ranges are supported (step=1)")
            if start < 0:
                raise ValueError(f"{name}: negative start column not supported")
            cols = (stop - start) if stop < 10**9 else None
            if cols is None:
                raise ValueError(f"{name}: a finite stop column is required")
            return {
                "Source": "file",
                "File": {"Path": str(path_str), "StartRow": 0, "StartCol": start, "Rows": rows or 0, "Cols": cols},
            }
        indep_block = _slice_to_block(indep_cols, str(path), "indep_cols")
        dep_block = _slice_to_block(dep_cols, str(path), "dep_cols")
        return indep_block, dep_block

    # ---- equation generator source ------------------------------------
    @staticmethod
    def from_equations(indep_equations: str, n_points: int, n_vars: int) -> dict:
        """Independent block from the CLI equation generator. `indep_equations` is a pipe-separated
        list, e.g. "0.001|(X-1)*1e-4"."""
        return {
            "Source": "generator",
            "Generator": {"Type": "equations", "DataPoints": int(n_points), "Variables": int(n_vars), "Equations": indep_equations},
        }

    def cleanup(self) -> None:
        for p in self._files:
            try:
                p.unlink(missing_ok=True)
            except OSError:
                pass