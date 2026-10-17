# Roadmap: shared chat library (CuteChat)

> Copy of CuteChat/docs/DESIGN.md at the time of extraction (2026-10-17); the library repo is the source of truth. Library checkout: `../CuteChat` or `external/CuteChat` (`-DCUTECHAT_DIR=`). SupraFit status: branch `feature/cutechat-extract`.


Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net> (GPL-3.0-or-later, like SupraFit and qurcuma).
Claude Generated 2026 (analysis of both code bases: SupraFit `feature/ui-redesign` @ 2ee86a7c, qurcuma `feature/llm-tools` @ f94f724).

## 1. What this is

A small Qt6 library holding everything about "an LLM agent that drives an application through
named tools" that is *not* specific to one application:

| Layer (target)            | Content | Qt modules |
|---------------------------|---------|------------|
| `cutechat_core`           | tool contract, registry, dispatcher, LLM client, config, presets, session (agent loop), chat log / history / recorder, persistence + host interfaces | Core, Network |
| `cutechat` (optional)     | `ChatDock` (bubbles, Markdown, streaming, reasoning, history, export), `EndpointDialog` | + Widgets |

Applications plug in through two small interfaces (`ChatHost`, `ChatBackend`); the library
contains no application header and no application colour.

## 2. Comparison of the two starting points

Both layers share one ancestor (qurcuma `docs/WP-llm-tool-layer.md`); SupraFit copied it on
2026-10-11 and then diverged. File-by-file result of `diff`:

| Area | qurcuma | SupraFit | CuteChat |
|---|---|---|---|
| `ToolSpec/ToolResult/ToolEffect/ToolAutonomy/ToolAffinity` | original | identical semantics (comments differ) | one copy; semantics unchanged |
| `ToolRegistry` (schema check, catalogue, `available`) | original, `instance()` singleton | same code, same `instance()` | one copy |
| `ToolDispatcher` | log hub line per call (`LogHub`), `callOrigin`, interrupt flag | `callOrigin`, interrupt polled in 50 ms slices (a Stop is heard within 50 ms), `recordRefusal`, no hub | SupraFit logic + optional **log sink** callback instead of `LogHub` |
| `LlmConfig`/`LlmProfile` | `updateBaseUrl`, no presets | presets, `save()` keeping unknown keys, `displayName`, `missingKeyMessage`, `isLocalEndpoint`, `LlmModelSelection` | SupraFit + `updateBaseUrl` (thin wrapper over load/save) |
| `LlmClient` | `reasoning effort` level, `describeModel` (one model), request timeout | `queryCapabilities` (all models), `modelListFailed` with readable messages, stale-reply protection, profile-switch clears model, bug-fixed cancel | SupraFit + `setReasoningEffort`, `describeModel` |
| `LlmSession` | vision (`setVisionCapable`, `showImage`), wait rounds do not count, hub notes, runs on a worker thread | autonomy inside the session, `keepHistory`, `setConversation`, deferred tool execution (`QTimer::singleShot(0)`), cancel keeps history valid, refusals audited | SupraFit + vision + free wait rounds + `note()` signal |
| Presets (Ollama, LM Studio, OpenAI, ...) | no | yes (`llmpresets`) | yes |
| Chat log, history, export/import, input history | no | yes (`chatlog`) | yes |
| Audit recorder (effect, result, duration, approval) | no (log hub text only) | yes (`chatrecorder`) | yes |
| Project-bound chat storage | no | `ProjectChats` in the project file | `ChatBackend` interface; SupraFit keeps `ProjectChats` |
| Chat dock | view onto an externally owned session (worker thread); collapsible reasoning per turn, thinking-effort and rounds boxes, editable endpoint, copy transcript | owns its engine; Markdown bubbles, tool lines, history menu, export/import, maximise, project-bound chats, endpoint manager | SupraFit dock + reasoning blocks + thinking/rounds controls (qurcuma features that are generic) |
| Endpoint manager dialog | no | yes (`EndpointDialog`, `SessionKeys`) | yes |
| Domain code | molecule tools, curcuma jobs, atom selection | project/analysis/inspect tools, post-fit jobs, headless runner | stays in the applications |

Things qurcuma has and SupraFit lacks that are generic (all ported): vision messages,
free "waiting" rounds, `reasoning effort`, per-model `describeModel`, `updateBaseUrl`,
call-origin set by the session, reasoning blocks and effort/rounds controls in the dock.

## 3. What stays in the applications

* All tools (`tools_*.cpp`), job classes, the system prompt text, and the approval *policy* if it
  is more than the default question box (qurcuma: per-tool "allow for this session").
* SupraFit: `ProjectChats` (storage inside the project file under `agent_chats`), the headless
  runner `agentrunner`, `postfitjobs`, the `Appearance` palette switch, `ensureModelWidget`-like
  GUI follow-up of a tool call (done through `ChatHost::toolFinished`).
* qurcuma: `LogHub`, the script dock bridge, the worker-thread wiring of the session.

## 4. Interfaces

```cpp
class ChatBackend {                 // persistence of chats bound to a context
public:
    virtual ~ChatBackend() = default;
    virtual QVector<ChatLog> load(const QString& contextId, QStringList* warnings = nullptr) = 0;
    virtual void store(const QString& contextId, const ChatLog& chat) = 0;   // insert or replace by id
    virtual void clear(const QString& contextId) = 0;
    virtual bool accepts(const QString& contextId) const { return !contextId.isEmpty(); }
};
class FileChatBackend : public ChatBackend { ... };   // <dir>/<context>/<chat id>.json, ships with the library

class ChatHost {                    // everything application specific the dock needs
public:
    virtual ~ChatHost() = default;
    virtual QString systemPrompt() const = 0;
    virtual void registerTools(ToolRegistry& registry) = 0;          // called once by the dock
    virtual ChatBackend* chatBackend() { return nullptr; }          // null: history in AppData only
    virtual bool storeInBackend() const { return true; }            // user setting "store chats in ..."
    virtual QString appVersion() const; virtual QString appCommit() const;  // written into the chat record
    virtual bool approveToolCall(const ToolSpec&, const QJsonObject&, QWidget* parent); // default: question box
    virtual void toolFinished(const QString& name, const QJsonObject& args, const ToolResult&) {}  // "state changed" hook
    virtual ChatUiTexts uiTexts() const;                            // title, placeholder, autonomy labels, context noun
    virtual QString settingsPrefix() const { return QStringLiteral("chat"); }   // QSettings keys
};
```

* **Context id**: the application calls `ChatDock::setContext(id)` whenever the thing a chat is
  bound to changes (SupraFit: project uuid; qurcuma: structure/session id). The dock then loads
  the context's most recent chat from the backend, saves on every finished turn, and offers the
  context's chats under *History > This project*. An empty id means "session-only chat".
* **Settings/config naming**: `llm.json` lives in `QStandardPaths::AppConfigLocation`
  (`QCoreApplication::organizationName/applicationName`), chat history in
  `AppDataLocation/chats`, `QSettings` keys under `settingsPrefix()` (default `chat/...`,
  identical to the keys SupraFit already wrote, so nothing migrates).
* **Tools**: `ToolSpec` semantics are unchanged; both code bases already agreed on them. The
  dock owns a `ToolRegistry`/`ToolDispatcher` unless the host passes its own
  (`ChatDock::setRegistry`, for `ToolRegistry::instance()` style applications).
* **Threads**: `LlmSession` and `LlmClient` are thread-agnostic (they only use queued signals and
  the dispatcher's marshalling). The dock runs them on the GUI thread (SupraFit). An application
  that wants the agent loop on a worker thread (qurcuma) keeps owning client and session and uses
  the core library only, until a `ChatEngine` wrapper exists (section 8).

## 5. Naming

* Names are kept global (`ToolSpec`, `LlmClient`, `ChatDock`, ...). Both applications use them in
  ~4000 lines of tool code; a namespace would be churn without a collision to prevent. Headers are
  reached as `<cutechat/xyz.h>` so a clash on the file name is impossible.
* The chat JSON keeps its keys (`project_uuids`, block key `agent_chats`) so stored SupraFit
  projects keep loading; the C++ API says *context* (`addContextId`, `contextIds`,
  `ChatLog::blockKey`).
* CMake: `cutechat_core`, `cutechat`, alias `CuteChat::core`, `CuteChat::cutechat`; options
  `CUTECHAT_BUILD_WIDGETS`, `CUTECHAT_BUILD_TESTS`, `CUTECHAT_INSTALL`.

## 6. Licence

GPL-3.0-or-later, identical to SupraFit and qurcuma, so code moves both ways without a
re-licensing step. `flowlayout` (Qt example code) keeps its original BSD header. Copyright stays
with Conrad Hübler; Claude-generated parts are marked in comments.

## 7. Migration plan

1. Library repo (this one) with the generic code and its tests (SupraFit's tests plus qurcuma's
   vision/wait-round tests).
2. SupraFit `feature/cutechat-extract`: `add_subdirectory` of the library, generic files removed from
   `src/agent` and `src/ui`, `SupraFitChatHost` (prompt, tools, project id) and `ProjectChatBackend`
   (`ProjectChats`) stay in SupraFit. Library tests are registered in the SupraFit build too.
3. qurcuma `feature/shared-chat`: `qurcuma_llm`/`core/tool*` replaced by the core library, qurcuma's
   dock keeps its own widgets (needs `ChatEngine` for the worker-thread model).
4. Share: git submodule `external/CuteChat` once a remote URL exists (SupraFit uses the same pattern
   for `external/CuteChart`); until then CMake looks for `-DCUTECHAT_DIR=...`, `external/CuteChat`,
   then a sibling checkout `../CuteChat`.

Migration notes for existing code: `#include "src/agent/xyz.h"` -> `<cutechat/xyz.h>`;
`ChatLog::projectKey/addProjectUuid/projectUuids/toProjectBlock/fromProjectBlock` ->
`blockKey/addContextId/contextIds/toBlock/fromBlock`; `LlmSession(client, registry, dispatcher,
hub, parent)` (qurcuma) -> `LlmSession(client, registry, dispatcher, parent)` plus
`ToolDispatcher::setLogSink` / `LlmSession::note` signal for the former `LogHub` lines.

## 8. Risks and open points

* **Worker-thread dock (qurcuma)**: the library dock is GUI-thread only. A `ChatEngine` class that
  owns client+session on a thread and exposes marshalled setters is needed before qurcuma can use
  `ChatDock` itself; qurcuma adopts the core now and keeps its dock.
* **Qt moc**: Qt 6.11 moc breaks with raw string literals containing `\"` before a Q_OBJECT class;
  the library uses plain concatenation.
* **Global names** can collide with an application class of the same name; none known today.
* **Remote URL**: not decided; until a remote exists both apps build against a local checkout.
* **Behaviour merge**: SupraFit's session defers tool execution out of the reply slot; in qurcuma the
  same code runs on a worker thread. Both are covered by tests (nested event loops in a tool, wake
  a tool waiting for the GUI thread).
