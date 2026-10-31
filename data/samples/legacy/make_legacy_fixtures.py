#!/usr/bin/env python3
"""Synthetic project files in the layouts older SupraFit versions wrote (Claude Generated, 2026).

The real files of older versions are the operator's data and stay out of the repository; these have the same layout, are small, and are
made so that a correct reader must reproduce them exactly (noise-free data: the sum of squares must come out as ~0).

  legacy_1599_nmr_1_1.suprafit   SupraFit 1.x/2.x: data as "concentrations"/"signals" strings, model as "1:1-Model", old parameter keys
  multi_project.suprafit         several projects in one file ("project_0", "project_1", ...), no version of its own
  nmr_any_grid_2003.json         a "nmr_any" model defined by the grid MaxA/MaxB (before the Reactions field), data of the 2003 reference
  kinetics_2003.json             bimolecular model with four parameters (before the scale factor) and the flexible model of August 2022 (other formula)
  itc_first_points.json          ITC fit that left out the first two points (stored sum of squares over the others), heat table with the first point unchecked

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


# --- kinetic models of 2003 --------------------------------------------------------------------------------------------------
import random

random.seed(7)


def table(header, values, cols=1):
    n = len(values)
    return {"checked": {str(i): "1" for i in range(n)}, "cols": cols, "data": {str(i): "%.9g" % v for i, v in enumerate(values)},
            "header": header, "rows": n}


def data_block(indep, dep, indep_header, dep_header, system=None):
    return {"DataType": 1, "SupraFit": 2003, "colors": "#209fdf", "content": "", "dependent": table(dep_header, dep),
            "independent": table(indep_header, indep), "raw": {}, "system": system or {}, "title": "synthetic", "uuid": "{00000000-0000-0000-0000-00000000000%d}",
            "xaxis": False}


def model_block(model, name, globals_, locals_, sse, options=None):
    return {"SSE": sse, "SAE": 0, "SupraFit": 2003, "colors": "#7f7fff", "converged": True, "keys": name, "mean_error": 0, "model": model, "name": name,
            "options": options or {}, "standard_error": 0, "valid": True, "variance": 0,
            "data": {"active_series": "", "globalParameter": {"checked": {"0": " ".join("1" for _ in globals_)}, "cols": len(globals_),
                     "data": {"0": " ".join("%.9g" % g for g in globals_)}, "header": "", "rows": 1},
                     "localParameter": {"checked": {"0": ""}, "cols": 0, "data": {"0": ""}, "header": "", "rows": 1},
                     "locked": " ".join("1" for _ in globals_), "methods": {}}}


t = [float(i) for i in range(0, 60, 2)]
k, cA0, cAeq, cc0 = 0.0042, 0.55, 0.05, 0.9
bimolecular = [(cA0 * (cA0 - cc0) * math.exp(k * x * (cA0 - cc0))) / (cA0 * math.exp(k * x * (cA0 - cc0)) - cc0) + cAeq + random.gauss(0, 0.002) for x in t]
sse_bi = 0.0
for x, y in zip(t, bimolecular):
    f = (cA0 * (cA0 - cc0) * math.exp(k * x * (cA0 - cc0))) / (cA0 * math.exp(k * x * (cA0 - cc0)) - cc0) + cAeq
    sse_bi += (y - f) ** 2
kf, n_, a0, aeq = 0.004, 1.3, 0.58, 0.06


def flex_old(x):
    return (a0 - aeq) * (((a0 - aeq) ** (n_ - 1)) * kf * x * (n_ - 1) + 1) ** (1 / (n_ - 1)) + aeq


flex = [flex_old(x) + random.gauss(0, 0.002) for x in t]
sse_flex = sum((y - flex_old(x)) ** 2 for x, y in zip(t, flex))
kinetics = {
    "project_0": {"data": data_block(t, bimolecular, "t", "c(A)"), "model_1": model_block(107, "Bimoleculare Kinetics", [k, cA0, cAeq, cc0], [], sse_bi)},
    "project_1": {"data": data_block(t, flex, "t", "c(A)"), "model_1": model_block(108, "Flexible kinetic model", [kf, n_, a0, aeq], [], sse_flex)},
}
open(os.path.join(here, "kinetics_2003.json"), "w").write(json.dumps(kinetics, indent=1))

# --- an ITC fit without the first two points ----------------------------------------------------------------------------------
V0, cell_c, syr_c = 1410.3, 0.1, 1.0
lgk, dh, dil_heat, dil_inter, fxx = 7.5, -4600.0, -1400.0, 0.1, 1.14
vol = [1.0] + [5.0] * 29


def itc_value(volumes):
    cell, prod, cp = cell_c * 1e-3, 1.0, 0.0
    out = []
    for v in volumes:
        prod *= 1 - v / V0
        cell *= 1 - v / V0
        guest0 = syr_c * 1e-3 * (1 - prod)
        host0 = cell * fxx
        kk = 10 ** lgk
        pp = host0 + guest0 + 1 / kk
        cx = (pp - math.sqrt(pp * pp - 4 * host0 * guest0)) / 2
        out.append(V0 * (cx - cp * (1 - v / V0)) * dh + guest0 * dil_heat + dil_inter)
        cp = cx
    return out


model_heat = itc_value(vol)
heat = [h + random.gauss(0, 0.05) for h in model_heat]
heat[0] = -0.01  # the first injection is an artefact
heat[1] = heat[1] * 0.78  # and the second one is off as well
sse_itc = sum((a - b) ** 2 for a, b in zip(heat[2:], model_heat[2:]))
itc_data = data_block(vol, heat, "Inject Volume", "1", {"1": "1410.3", "2": "0.1", "3": "1", "4": "298.154", "5": "true"})
itc_data["dependent"]["checked"]["0"] = "0"
itc = {"data": itc_data, "model_1": model_block(10, "ITC 1:1-Model", [lgk], [], sse_itc, {"2": "auto"})}
itc["model_1"]["data"]["localParameter"] = {"checked": {"0": "1 1 1 1"}, "cols": 4, "data": {"0": "%g %g %g %g" % (dh, dil_heat, dil_inter, fxx)}, "header": "", "rows": 1}
json.dump(itc, open(os.path.join(here, "itc_first_points.json"), "w"), indent=1)
