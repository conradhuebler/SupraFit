<!--
Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
Claude Generated - work package / roadmap document
-->

# Work Package: Main Window Redesign

**Status:** WIP · **Branch:** `feature/ui-redesign` · **Owner:** Conrad Hübler · **Author note:** Claude Generated

## Goal

A clearer, more illustrative main window: fewer large button blocks, more room for the model tab and
the charts, one look that works in light and dark mode and on small screens (1366x768, ideally
1024x600). No feature loss: every action stays reachable, shortcuts stay.

Input: the operator's annotated screenshot [docs/ui_redesign/feedback_mockup.png](../docs/ui_redesign/feedback_mockup.png);
the pre-redesign state is captured in `docs/ui_redesign/before_*.png`.

## Mockup items

1. **Toolbar** — move the groups up beside each other and use submenus to save space:
   New (Window, Table) | Open (Project, Thermogram, Spectra) | Save (Save, Save As) | Settings & Info.
   The two-row "Workspace" button block becomes menus:
   `Data → Edit, Split | Model → Experiment (NMR, ITC, UV/Vis, Fluorescence) → Stoichiometry | Evaluation → Statistics, Analyse and Compare`.
2. **Left column** — "Projects" with the "Log" (message dock) stacked underneath, instead of the log
   taking the full-width bottom area.
3. **Model tab** — numbered sections 1 "Fit model", 2 "Model Options", 3 "System Parameter";
   "Datenreihen" (data series) and "Ergebnisse des Fits" (fit results) beside the options instead of
   below; keep the button row (Scan … Charts) prominent and grouped.

## Assumptions (mockup is a sketch; most conservative reading)

- A1: The workspace menus (Data/Model/Evaluation) move into the single main toolbar row and act on the
  project that is currently shown; without a project they are disabled. The per-project workspace
  header shrinks to nothing (its actions/menus still live in `MDHDockTitleBar`).
- A2: Group buttons open their menu; where a group has an obvious primary action (Open Project,
  Save Project, New Table) the button runs it on click and offers the rest via its arrow.
- A3: "Optimise All", "New Model", "Load Model file", "Any Model" and "Remove All" are not named in
  the pink hierarchy; they go into Model/Evaluation where they belong by meaning.
- A4: Log stays a toggleable panel (F4) with the unread indicator on its toggle action; it is a
  vertical split under the project tree, not a free dock.
- A5: No QMenuBar (would add a row); all actions with shortcuts are registered on the window so the
  shortcuts work although the actions sit in drop-down menus.

## Work packages (one buildable commit each)

- **WP1** Toolbar/menu restructure (item 1) — `suprafitgui.cpp`, `modeldataholder.cpp`.
- **WP2** Left column Projects + Log (item 2) — `suprafitgui.cpp`, `messagedock.cpp`.
- **WP3** Model tab layout (item 3) — `modelwidget.cpp`.
- **WP4** Polish: spacing, palette-aware colours (no hard-coded light colours), tooltips that say what
  a control is, fix the malformed main-window stylesheet.
- **WP5** Appearance setting `System / Light / Dark` (settings registry → ConfigDialog), applied at
  runtime via Fusion + `QPalette`; charts follow the theme (background, axes, labels, grid).
- **WP6** Small screens: usable at 1366x768 (ideally 1024x600): no large fixed minimum sizes, scroll
  areas for tall content, compact toolbar mode, no forced horizontal scrollbar.

Verification per WP: build, offscreen render (`QT_QPA_PLATFORM=offscreen`, screenshots in
`docs/ui_redesign/`), interactions that cannot be checked headless are listed in the commit/report.

## Status

- WP0 design note + mockup: 🤖 committed
- WP1 toolbar/menus: 🤖 ⚙️ build + offscreen render (`docs/ui_redesign/wp1_*.png`); clicks/shortcuts not exercised headless
