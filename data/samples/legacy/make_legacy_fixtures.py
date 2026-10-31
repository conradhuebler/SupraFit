#!/usr/bin/env python3
"""Synthetic project files in the layouts older SupraFit versions wrote (Claude Generated, 2026).

The real files of older versions are the operator's data and stay out of the repository; these have the same layout, are small, and are
made so that a correct reader must reproduce them exactly (noise-free data: the sum of squares must come out as ~0).

  legacy_1599_nmr_1_1.suprafit   SupraFit 1.x/2.x: data as "concentrations"/"signals" strings, model as "1:1-Model", old parameter keys
  multi_project.suprafit         several projects in one file ("project_0", "project_1", ...), no version of its own
  nmr_any_grid_2003.json         a "nmr_any" model defined by the grid MaxA/MaxB (before the Reactions field), data of the 2003 reference

Run from this directory: ./make_legacy_fixtures.py
"""
import json, math, zlib, os

here = os.path.dirname(os.path.abspath(__file__))
samples = os.path.join(here, "..")


def qcompress(obj):
    raw = json.dumps(obj, separators=(",", ":")).encode()
    return len(raw).to_bytes(4, "big") + zlib.compress(raw, 9)


def one_to_one_signal(a0, b0, lgk, d_free, d_complex):
    k = 10 ** lgk
    # AB from AB = k (A0 - AB)(B0 - AB)
    p = a0 + b0 + 1 / k
    ab = (p - math.sqrt(p * p - 4 * a0 * b0)) / 2
    return d_free + (d_complex - d_free) * ab / a0


# --- SupraFit 1.x/2.x single project ---------------------------------------------------------------------------------------
a0, lgk, d0, d1 = 0.001, 3.2, 7.10, 8.45
conc, sig = {}, {}
for i in range(12):
    b0 = a0 * (0.1 + 0.45 * i)
    conc[str(i)] = "%g %g" % (a0, b0)
    sig[str(i)] = "%.9g" % one_to_one_signal(a0, b0, lgk, d0, d1)
legacy = {
    "data": {
        "colors": "#209fdf",
        "concentrations": conc,
        "datatype": "discrete",
        "header": "Host (A)|Guest (B)|1",
        "signals": sig,
        "system": {},
    },
    "model_1": {
        "SupraFit": 1599,
        "colors": "#7f7fff",
        "converged": True,
        "data": {
            "globalParameter": {"0": "%g" % lgk, "names": "K<sub>11</sub>"},
            "localParameter": {"0": "%g %g" % (d0, d1), "names": "A|AB"},
            "statistics": {},
        },
        "keys": "1:1-Model",
        "mean_error": 0.0,
        "model": "1:1-Model",
        "options": {},
        "runtype": 3,
        "standard_error": 0.0,
        "sum_of_squares": 0.0,
    },
}
open(os.path.join(here, "legacy_1599_nmr_1_1.suprafit"), "wb").write(qcompress(legacy))

# --- several projects in one file --------------------------------------------------------------------------------------------
script = json.load(open(os.path.join(samples, "saved.json")))
reference = json.load(open(os.path.join(samples, "reference", "simulated_1_1.json")))
multi = {"project_0": script, "project_1": reference}
open(os.path.join(here, "multi_project.suprafit"), "wb").write(qcompress(multi))

# --- nmr_any defined by the grid ---------------------------------------------------------------------------------------------
grid = json.loads(json.dumps(reference))
for key in [k for k in grid if k.startswith("model_") and k != "model_2"]:
    del grid[key]  # model_2 is the 1:1/1:2 model, the grid MaxA=1, MaxB=2 the same species
m = grid["model_2"]
m["model"] = 34
m["options"] = {}  # the options of the fixed model mean other things; the any model starts with every species on
m["ModelDefinition"] = {"ModelDefinition": {"MaxA": 1, "MaxB": 2}}
# the fixed model has the stepwise constants lg K11, lg K12, the any model the cumulative lg beta(AB) = lg K11, lg beta(AB2) = lg K11 + lg K12
gp = m["data"]["globalParameter"]
k11, k12 = (float(x) for x in gp["data"]["0"].split())
gp["data"]["0"] = "%.9g %.9g" % (k11, k11 + k12)
json.dump(grid, open(os.path.join(here, "nmr_any_grid_2003.json"), "w"), indent=1)
