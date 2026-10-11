# SupraFit Model ID Reference

This document provides a reference for the model IDs used in SupraFit. The model IDs are defined as an enum in `src/global.h`.

| Model ID | Model Name |
|---|---|
| 0 | Data |
| 1 | ¹H 1:1-Model |
| 2 | ¹H 2:1/1:1-Model |
| 3 | ¹H 1:1/1:2-Model |
| 4 | ¹H 2:1/1:1/1:2-Model |
| 5 | Michaelis Menten Kinetics |
| 6 | Monomoleculare Kinetics |
| 7 | Arrhenius Plot |
| 8 | Eyring Plot |
| 10 | ITC 1:1-Model |
| 11 | ITC 2:1/1:1-Model |
| 12 | ITC 1:1/1:2-Model |
| 13 | ITC 2:1/1:1/1:2-Model |
| 14 | Independent Multiple Site |
| 15 | Two Set Multiple Site |
| 16 | Blank Titration |
| 17 | flexible ITC Model |
| 20 | Φ 1:1-Model |
| 21 | Φ 2:1/1:1-Model |
| 22 | Φ 1:1/1:2-Model |
| 23 | Φ 2:1/1:1/1:2-Model |
| 30 | UV/VIS 1:1-Model |
| 31 | UV/VIS 2:1/1:1-Model |
| 32 | UV/VIS 1:1/1:2-Model |
| 33 | UV/VIS 2:1/1:1/1:2-Model |
| 34 | ¹H flexible NMR-Model |
| 35 | flexible UV/VIS Model |
| 100 | ScriptModel |
| 101 | Indep. Quadrat |
| 102 | Dep. AnyModel |
| 103 | Decay Rates |
| 107 | Bimoleculare Kinetics |
| 108 | Flexible kinetic model |
| 109 | TIAN Thermokinetic |
| 110 | Monomolecular Kinetics with Evaporation |
| 111 | BET Absorption isotherm |
| 200 | Meta Model |
| 404 | Unknown |

## Notes

The "flexible" models — **34** (`nmr_any`), **35** (`uvvis_any`) and **17** (`itc_any`) — are defined
by a free-text *Reactions* field (arrow syntax, e.g. `A + B <=> AB`, `2 A <=> A2`, `A + C <=> AC`)
parsed by `ReactionParser` into an equilibrium system solved by the general `ConcentrationSolver`.
`nmr_any`/`uvvis_any` support an **arbitrary number of components** (one independent concentration
column per component); `itc_any` stays 2-component (host/guest totals come from the cell/syringe
protocol) but allows arbitrary species. The *Reactions* field is **required** — an empty field leaves
the model undefined (the legacy `MaxA`/`MaxB`/`MaxSelfA`/`Species` grid has been removed). Claude Generated.
