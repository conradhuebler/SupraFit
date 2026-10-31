# Fluorescence models: signal, inner filter, quenching

> **AI-implemented.** The rebuild of the fixed fluorescence models and the two corrections (inner filter, dynamic quenching) were written by an AI
> (Claude, Anthropic, 2026) at the operator's request. The equations were derived and tested against independent code (see *Tests*), **not** against
> experiments, and the operator has **not yet checked the science against the literature**. References below are quoted from memory and must be
> verified before the models are used in a publication. Every place that carries these terms in the program says so (option names, model output,
> code comments).

Code: `src/core/models/titrations/fluorescence/` (`fl_any_Model`, the four fixed models are presets of it). Tests: `test_fl_fixed_vs_any`,
`test_fl_corrections`, `test_fl_any_equivalence`.

## 1. Signal

For titration point *i* and series *j* (an emission wavelength, or a channel) the signal of the model without corrections is

    F_ij = sum_s c_is * phi_sj

with *s* over the free components (host A, guest B, ...) and over all species (AB, AB2, A2B, ...), *c_is* their equilibrium concentrations at point *i*
and *phi_sj* the fluorescence coefficient of species *s* in series *j* (molar fluorescence, including the instrument factor; fitted, local parameters).
This is the linear regime of fluorescence: the signal is proportional to the concentration of the emitting species (low absorbance, no
re-absorption). It is the same form that is used for UV/Vis (Beer-Lambert) and it is the form of the usual fluorescence fits (Thordarson 2011).

Because the signal is linear in *phi*, the coefficients can be projected out of the fit (VarPro, option `FitSolver`): only the stability constants
remain as nonlinear parameters.

### Equilibria

The concentrations come from the reaction system (`SpeciationEngine`): cumulative stability constants beta_s (`lg beta` is the global parameter) and
the mass balances of the components. The fixed models are presets of the reactions:

| Model | id | reactions | global parameters |
|---|---|---|---|
| 1:1 | 20 | `A + B <=> AB` | lg beta(AB) |
| 2:1 / 1:1 | 21 | `A + B <=> AB`, `2 A + B <=> A2B` | lg beta(AB), lg beta(A2B) |
| 1:1 / 1:2 | 22 | `A + B <=> AB`, `A + 2 B <=> AB2` | lg beta(AB), lg beta(AB2) |
| 2:1 / 1:1 / 1:2 | 23 | all three | lg beta(AB), lg beta(A2B), lg beta(AB2) |

The earlier fixed models used stepwise constants (lg K11, lg K12, lg K21); the cumulative constants are lg beta(AB2) = lg K11 + lg K12 and
lg beta(A2B) = lg K11 + lg K21. Files written by the earlier models have another parameter layout and are not reproduced: they are read, their
parameters are reset and the report of the load says so.

## 2. Inner filter correction (option "Inner filter correction (AI-implemented)")

In a fluorescence cuvette the sample absorbs the exciting light on its way to the observed volume (primary inner filter) and re-absorbs part of the
emitted light (secondary inner filter). Both reduce the observed signal and grow with the absorbance, hence with the concentrations in the titration:
if they are ignored, the titration curve is bent and the constants are biased (shown in `test_fl_corrections`: lg beta 4.062 instead of 4.000 at
A_ex + A_em = 0.5).

**Model.** With a path length *d* (cm) and the absorbances at the excitation and at the emission wavelength

    A_ex,i = d * sum_c eps_ex,c * T_ic        A_em,i = d * sum_c eps_em,c * T_ic

(eps in L mol-1 cm-1, T_ic the total concentration of component *c* at point *i*, as given in the independent table), the observed signal is

    F_obs,i = F_i * 10^( -(A_ex,i + A_em,i) / 2 )

This is the widely used approximation for a standard 1 cm cuvette observed at 90 degrees, in which the observed volume is taken at the centre of the cell
(then the light passes half of the sample on both sides; Lakowicz 2006, Kubista et al. 1994). Everything in the factor is known (path length,
absorptivities and totals), so it is a fixed factor per point: the signal stays linear in *phi* and VarPro still applies.

**Inputs** (system parameters of the model, 0 means "does not absorb"): `Path length`, and for each component `<component> absorptivity (excitation)`
and `... (emission)`. They are measured separately (UV/Vis of the components at the two wavelengths).

**Assumptions and limits - to be judged by the operator:**

- The absorbance of the sample is the sum of the absorbances of the *components* (total concentrations), i.e. a complex absorbs like its parts. A
  spectral change upon binding (the usual reason for UV/Vis titrations) is not covered; then measured absorbances of the titration are needed and
  the data should be corrected before the fit.
- One emission absorptivity per component for all series: valid for one emission wavelength or a narrow band. Multi-wavelength series need absorptivities
  per series (not implemented).
- The 10^(-(A_ex + A_em)/2) form is an approximation that is good for absorbances of the order of 0.1 and gets worse above about 0.3 to 0.5; at high
  absorbance the exact result depends on the geometry of the instrument (Kubista et al. 1994 for an experimental correction). The honest advice is
  to dilute the sample first.
- Light scattering and the dependence on the position of the observed volume (effective geometry) are not modelled.

## 3. Dynamic quenching (option "Dynamic quenching by free (AI-implemented)")

Collisional (dynamic) quenching by a quencher Q reduces the signal according to the Stern-Volmer equation F0/F = 1 + K_SV [Q] (Lakowicz 2006,
chapter on quenching). Here

    F_obs,i = F_i / ( 1 + K_SV * [Q]_i )

with [Q]_i the **free** equilibrium concentration of the component chosen in the option (the guest by default in a host-guest titration), and
lg K_SV the last global parameter (L mol-1; fitted only if the option names a component, otherwise unused).

**Assumptions and limits:**

- All emitting species are quenched with the same K_SV.
- Static quenching (a non-fluorescent complex) is *not* an extra term: it is part of the binding model, a complex with a lower coefficient
  phi_complex. Dynamic and static quenching by the same agent cannot be told apart from one titration curve: K_SV, K and phi_complex are correlated.
  K_SV should be determined independently (titration of the fluorophore with the quencher without the host) and compared with the fitted value.
- Mixed static and dynamic quenching with upward curvature (sphere of action, (1 + K_D[Q]) (1 + K_S[Q])) is not modelled.
- Quenching by a component that is not free in solution (bound in a complex) is not included; [Q] is a free concentration.

## 4. Implementation

`fl_any_Model::CalculateConcentrations` builds the design matrix `m_design = diag(g q) * C`, with `C` the concentrations of the free components and
the species, `g_i` the inner filter factor and `q_i = 1 / (1 + K_SV [Q]_i)`. The signal is `m_design * phi^T`. `ProjectLinearParameters` (VarPro) and
`UpdateShifts` (initial guess) use the same matrix. Without the options `g = q = 1` and the model is the one it was before.

Options: `InnerFilter` (100), `Quenching` (101; values `none` and the component names). System parameters: `PathLength` (2000),
`EpsilonExcitation + c` (2100 + c), `EpsilonEmission + c` (2200 + c).

## 5. Tests

`test_fl_corrections` makes the data with independent code in the test (formulas above, free guest from the model without corrections) and requires that the
fit with the correction recovers lg beta (and lg K_SV) to 1e-3 / 5e-2 and describes the data (SSE/energy < 1e-12), that the fit without it is measurably
biased (lg beta 4.062 for the inner filter, 5.578 for a quenching of lg K_SV = 5), and that VarPro and the full-vector solver agree with both corrections
on. `test_fl_fixed_vs_any` pins the defects of the earlier fixed models (first point with its own coefficient; fl_ItoI scaling with the model signal
of row 0) and that the presets recover the constants of textbook data. **What the tests cannot show:** that the corrections describe a real
instrument. For that, data with known absorbances (a dye series) and a known quencher are needed.

## 6. References (from memory, to be verified)

- J. R. Lakowicz, *Principles of Fluorescence Spectroscopy*, 3rd ed., Springer 2006 (quenching, Stern-Volmer; inner filter effect).
- M. Kubista, R. Sjoeback, S. Eriksson, B. Albinsson, Analyst 1994, 119, 417 (experimental correction for the inner-filter effect).
- P. Thordarson, Chem. Soc. Rev. 2011, 40, 1305 (determining association constants from titration experiments).
- D. B. Hibbert, P. Thordarson, Chem. Commun. 2016, 52, 12792 (data analysis in supramolecular chemistry).
