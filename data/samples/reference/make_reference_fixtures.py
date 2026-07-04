#!/usr/bin/env python3
"""Build slim reference fixtures for the real-measuring regression tests.

Claude Generated (2026). Reads full SupraFit `.suprafit` projects that were analysed
with a trusted, stable SupraFit version (the PeerJ Monte-Carlo paper set: Hübler C. 2022,
PeerJ Analytical Chemistry 4:e23), and writes small `.json` projects that keep everything
the tests need as an oracle — the fitted parameters and, for every post-processing method
(Monte Carlo / Cross-Validation / Reduction / FastConfidence), the *summary* statistics
(boxplot mean/median/quantiles/whiskers/stddev + confidence interval + point value) — while
dropping the multi-megabyte raw resampling clouds and histogram bins.

Usage:
    make_reference_fixtures.py <SOURCE_DIR> [OUT_DIR]

<SOURCE_DIR> holds the original "Simulated * Experiment.suprafit" files. Output defaults to
this script's directory. Re-run to regenerate after producing new trusted analyses.
"""
import json
import os
import sys
import zlib

# result-block keys to keep; everything else (raw cloud, x/y histogram bins) is dropped
BLOCK_KEEP = {"boxplot", "confidence", "value", "name", "index", "type"}
# boxplot keys to keep; the mild/extreme outlier lists are dropped (they are raw-ish bulk)
BOX_KEEP = {
    "count", "mean", "median",
    "lower_quantile", "upper_quantile",
    "lower_whisker", "upper_whisker",
    "stddev", "std",
}

# map the trusted source file -> committed fixture name
FILES = {
    "Simulated 1_1 Experiment.suprafit": "simulated_1_1.json",
    "Simulated 1_1_1_2 Experiment.suprafit": "simulated_1_1_1_2.json",
    "Simulated 2_1_1_1 Experiment.suprafit": "simulated_2_1_1_1.json",
    "Simulated 2_1_1_1_1_2 Experiment.suprafit": "simulated_2_1_1_1_1_2.json",
}


def load_suprafit(path):
    raw = open(path, "rb").read()
    try:
        return json.loads(zlib.decompress(raw[4:]))  # Qt qCompress: 4-byte length header
    except zlib.error:
        return json.loads(raw)  # already plain JSON


def slim(project):
    for mk in [k for k in project if k.startswith("model")]:
        methods = project[mk].get("data", {}).get("methods", {})
        if not isinstance(methods, dict):
            continue
        for mo in methods.values():
            mo.get("controller", {}).pop("raw", None)
            for pk in [k for k in mo if k != "controller"]:
                blk = mo[pk]
                if not isinstance(blk, dict):
                    continue
                for k in list(blk):
                    if k not in BLOCK_KEEP:
                        blk.pop(k)
                box = blk.get("boxplot")
                if isinstance(box, dict):
                    for k in list(box):
                        if k not in BOX_KEEP:
                            box.pop(k)
    return project


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    src_dir = sys.argv[1]
    out_dir = sys.argv[2] if len(sys.argv) > 2 else os.path.dirname(os.path.abspath(__file__))
    for src_name, out_name in FILES.items():
        src = os.path.join(src_dir, src_name)
        if not os.path.exists(src):
            print(f"skip (missing): {src}")
            continue
        project = slim(load_suprafit(src))
        out = os.path.join(out_dir, out_name)
        json.dump(project, open(out, "w"), separators=(",", ":"))
        print(f"{out_name}: {os.path.getsize(out) // 1024} KB")


if __name__ == "__main__":
    main()
