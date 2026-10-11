"""Model name <-> id mapping.

The CLI dispatches `AddModels` by the integer id (the dict key is only a log label), and the
C++ `Name2Model` uses colon display names ("nmr_1:1-Model"), not snake_case. This module owns the
snake_case -> id table sourced from `enum SupraFit::Model` in src/global.h, so Python users can
write `add_model("nmr_1_1")`. Claude Generated.
"""

from __future__ import annotations

from .errors import ModelNameError

# snake_case name -> SupraFit::Model id (src/global.h:156-194). Kept in sync with that enum.
MODELS: dict[str, int] = {
    # NMR
    "nmr_1_1": 1,
    "nmr_2_1_1_1": 2,
    "nmr_1_1_1_2": 3,
    "nmr_2_1_1_1_1_2": 4,
    "nmr_any": 34,
    # ITC
    "itc_1_1": 10,
    "itc_2_1_1_1": 11,
    "itc_1_1_1_2": 12,
    "itc_2_1_1_1_1_2": 13,
    "itc_n_1_1": 14,
    "itc_n_1_2": 15,
    "itc_blank": 16,
    "itc_any": 17,
    # Fluorescence
    "fl_1_1": 20,
    "fl_2_1_1_1": 21,
    "fl_1_1_1_2": 22,
    "fl_2_1_1_1_1_2": 23,
    # UV-Vis
    "uv_vis_1_1": 30,
    "uv_vis_2_1_1_1": 31,
    "uv_vis_1_1_1_2": 32,
    "uv_vis_2_1_1_1_1_2": 33,
    "uvvis_any": 35,
    # Kinetics / thermodynamics / other
    "michaelis_menten": 5,
    "monomolecular": 6,
    "arrhenius": 7,
    "eyring": 8,
}

# Reverse map (id -> canonical snake_case name); the first name registered for an id wins.
ID_TO_NAME: dict[int, str] = {mid: name for name, mid in MODELS.items()}


def model_id(name_or_id) -> int:
    """Resolve a model name (str) or id (int) to the integer SupraFit::Model id."""
    if isinstance(name_or_id, bool):  # bool is an int subclass; reject explicitly
        raise ModelNameError(f"model id must be a name or int, not bool: {name_or_id!r}")
    if isinstance(name_or_id, int):
        if name_or_id in ID_TO_NAME:
            return name_or_id
        raise ModelNameError(f"unknown model id: {name_or_id}")
    if isinstance(name_or_id, str):
        key = name_or_id.strip()
        if key in MODELS:
            return MODELS[key]
        # tolerate the C++ display names too, as a convenience
        raise ModelNameError(f"unknown model name: {name_or_id!r}; known: {sorted(MODELS)}")
    raise ModelNameError(f"model name/id must be str or int, got {type(name_or_id).__name__}")


def model_name(name_or_id) -> str:
    """Resolve a model name or id to its canonical snake_case name."""
    return ID_TO_NAME[model_id(name_or_id)]