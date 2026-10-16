<!--
Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
Claude Generated - verification record (UI redesign WP11)
-->

# Agent chats stored in the project — round-trip evidence (2026-10-16)

Requirement: "der chat muss im projekt mit gespeichert werden" — the conversation with the analysis
agent, including what the agent did, is part of the project file. Format: `agent_chats` in
[JSON_FORMAT_SPECIFICATION.md](../JSON_FORMAT_SPECIFICATION.md). Config and data directories of all
runs were scratchpad copies (XDG_CONFIG_HOME / XDG_DATA_HOME), local Ollama on 127.0.0.1:11434.

## 1. Headless run (suprafit_cli), saved with -o

```
suprafit_cli -i data/samples/reference/simulated_1_1.json \
  --agent "Refit the 1:1 model of the loaded project with fit_model and report lgK and SSE in one short table." \
  -o wp11_cli_out.json
[agent] profile local model glm-5.2:cloud | tool catalogue 16101 bytes
[tool] list_projects -> ok   [tool] list_models -> ok   [tool] fit_model -> ok   [tool] get_model_parameters -> ok
[agent] conversation stored in 1 project(s); save them (-o) to keep it
[agent] project with the conversation saved to .../wp11_cli_out.json
```

The saved file has the keys `agent_chats, data, model_0..model_3`; one chat, format version 2,
`app_version nightly-43-g5f17bd2a`, `git_commit 5f17bd2a`, `profile local`, `endpoint_host 127.0.0.1`,
`model glm-5.2:cloud`, `autonomy in-program`, `projects [{65080ac3-…}]`, 11 conversation messages,
no `api_key` anywhere. Audit trail:

| kind | tool | effect | approval | decided by | ms | result chars |
|---|---|---|---|---|---|---|
| user | | | | | | |
| tool | list_projects | read | not_required | autonomy:in-program | 0 | 138 |
| tool | list_models | read | not_required | autonomy:in-program | 0 | 1032 |
| tool | fit_model | compute | not_required | autonomy:in-program | 1 | 277 |
| tool | get_model_parameters | read | not_required | autonomy:in-program | 0 | 743 |
| assistant | | | | | | |

## 2. Reloaded and continued

- CLI: `suprafit_cli -i wp11_cli_out.json --agent "Which models does the project have? One line." -o wp11_cli_out2.json`
  → the file holds both chats (the first unchanged, the second new).
- GUI (offscreen): `wp11_cli_out2.json` opened; the dock shows the project's latest chat with the
  notice "stored with this project", the History menu lists both under "This project"
  (`docs/ui_redesign/wp11_history_menu_*.png`). A follow-up question in that chat ("Which model of this
  project has the lowest AIC?", glm-5.2:cloud) was answered with the restored context; Save Project
  wrote it back into the same file (that chat went from 4 to 6 entries), `projectsWithUnsavedAttachments`
  went from 1 to 0.

## 3. Found on the way

- GUI "Save Project" never wrote a single project: `saveProject(projectId, file)` had its arguments
  swapped (the signature is `saveProject(file, projectId)`); fixed.
- After loading a `.json`/`.suprafit` project, Save asked for a file name every time; it now writes back
  to the loaded file.
- Multi-project save wrote only the data block (no models); it now writes the complete project.
- `ReferenceProjectsTest` fails 2 cases (SSE and a CV mean of the 2:1 models) with and without these
  changes (checked by building it against the previous `projectmanager.cpp`); not caused by WP11.
