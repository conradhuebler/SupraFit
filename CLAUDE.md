# SupraFit Development Guide

## Project Overview
SupraFit is a C++/Qt framework for supramolecular chemistry analysis with statistical post-processing and Machine Learning integration.

## General Instructions

- Each source code dir has a CLAUDE.md with basic information of the code and logic
- **Keep CLAUDE.md files FOCUSED and CONCISE** - ONE clear idea per bullet, max 1-2 lines
  - ❌ DON'T: Multi-paragraph explanations, code examples, historical details
  - ✅ DO: Brief statements with links to detailed docs if needed
  - ✅ DO: "✅ **Feature name** - Brief description" for completed items
- Remove completed/resolved items after 2-3 updates (move to git history)
- Tasks corresponding to code must be placed in the correct CLAUDE.md file
- Each CLAUDE.md has a variable part (short-term info, bugs) and preserved part (permanent knowledge)
- **Instructions blocks** contain operator-defined future tasks and visions for code development
- Only include information important for ALL subdirectories in main CLAUDE.md
- Preserve new knowledge from conversations but keep it brief
- Always suggest improvements to existing code
- **Keep entries concise and focused to save tokens**
- **Keep git commits concise and focused**
- **Rule of thumb**: If a CLAUDE.md section exceeds 20 lines, consider if it's better placed elsewhere

## Build System
- **Build**: Use `/build/debug` or `/build/release` for out-of-source builds
- **CMake**: `cmake .. && make -j4`
- **Tests**: `make test`

## Development Standards

### Educational-First Design
- Clear, direct access to computational chemistry implementations
- Minimal abstraction layers that obscure scientific content
- Algorithm transparency with mathematical/physical implementations easily locatable
- Function/variable names reflecting scientific meaning

### Implementation Rules
- Mark new functions as "Claude Generated"
- Document functions (doxygen ready) with scientific context
- Document existing undocumented functions if appearing regularly (briefly and doxygen ready)
- Remove TODO Hashtags and text if done and approved
- Use modern Qt6 patterns, avoid deprecated functions
- Comprehensive error handling and logging
- Debug output: `qDebug()` within `#ifdef DEBUG_ON #endif`
- Console output: Use `fmt`, avoid `std::cout`
- **Always check instructions blocks** in relevant CLAUDE.md files
- Reformulate and clarify task and vision entries if not already marked as CLAUDE formatted
- In case of compiler warning for deprecated functions, replace the old function call with the new one
- Implement timing analysis for complex functions
- Keep track of significant improvements in AIChangelog.md, one line per fact
- **Complex Architecture Documentation**: Factory patterns, dispatchers, and multi-step workflows require comprehensive inline documentation
- Maintain backward compatibility where possible

### Copyright and File Headers
- **Copyright ownership**: All copyright remains with Conrad Hübler as the project owner
- **Year updates**: Always update copyright year to current year when modifying files
- **Claude contributions**: Mark Claude-generated code sections but copyright stays with Conrad
- **Format**: `Copyright (C) 2016 - 2025 Conrad Hübler <Conrad.Huebler@gmx.net>`
- **AI acknowledgment**: Add Claude contribution notes in code comments, not copyright headers

### Important Notes
- nullptr-checks are good things, however if there is a crash, prevent the crash by checks doesn't solve the origin of the nullptr

### Workflow States
- **ADD**: Features to be added
- **WIP**: Currently being worked on  
- **ADDED**: Basically implemented
- **TESTED**: Works (by operator feedback)
- **APPROVED**: Move to changelog, remove from CLAUDE.md

## Documentation

- [JSON Data Structure and I/O Functions](./JSON_datastruture.md)
- [Statistical Post-Processing Configuration](./src/capabilities/STATISTICAL_POSTPROCESSING.md)
- [Model ID Reference](./src/core/models/MODEL_ID_REFERENCE.md)
- [ML Integration Plan](./ml_integration_plan/README.md)
- [CLI Usage Examples](./src/client/usage_example.md)

## Key Components

### Core Libraries
- **libcore.a**: Core functionality (capabilities/, core/) with ProjectManager integration
- **libmodels.a**: Data models and analysis models
- **suprafit_cli**: Command-line interface with unified ProjectManager
- **suprafit**: GUI application with completed ProjectManager integration


## File Structure
```
src/
├── capabilities/    # DataGenerator, JobManager
├── client/         # CLI and ML pipeline
├── core/           # Core functionality
├── tests/          # Test suite
└── ui/             # GUI components
```

## Dependencies
- **Qt6**: Core, Test, Qml modules
- **Eigen**: Matrix operations
- **fmt**: Modern C++ formatting
- **ChaiScript**: Scripting support

## CLAUDE.md Organization
- Each `src/` subdirectory has detailed CLAUDE.md documentation
- **Keep entries concise and focused to save tokens**
- **Preserved Section**: Permanent documentation (don't edit)
- **Variable Section**: Short-term information (update regularly)
- **Instructions Block**: Operator-defined future tasks and visions

### Documentation Update Rules
- **Replace debugging details with architecture decisions** when issues are resolved
- **Remove unnecessary pointer addresses and crash investigation specifics**
- **Focus on architectural clarity** rather than technical debugging information
- **Document the "why" behind design decisions** for future reference
- **Eliminate redundant information** that doesn't add architectural value
- **Prioritize clean, maintainable documentation** over verbose troubleshooting history


## Build & Git Management

### Build Directories
- **`debug/`** - Development build: full debug symbols, no optimizations, slower runtime
  - Use: Testing features, debugging crashes, development workflow
  - Build: `cmake --build debug 2>&1 | tail -20` for quick status
  - Executable: `./debug/bin/linux/suprafit` (GUI) or `./debug/bin/linux/suprafit_cli` (CLI)

- **`release/`** - Optimized build: stripped symbols, O3 optimizations, fast runtime
  - Use: Performance testing, final deployment, production runs
  - Build: `cmake --build release 2>&1 | tail -20`
  - Executable: `./release/bin/linux/suprafit` (GUI) or `./release/bin/linux/suprafit_cli` (CLI)

### Git Best Practices
- **Only commit source files**: Use `git add <file>` for specific files, never `git add -A` without review
- **Review before committing**: Always check `git diff` and `git status` to avoid accidental commits
- **Build before commit**: Ensure `make -j4` succeeds and no compiler warnings/errors exist
- **Commit message format**: Start with action verb (Fix, Add, Improve, Refactor), follow with brief description
- **Include Co-Author info**: All commits include Claude contribution notes with proper attribution
- **Test artifacts stay local**: Build outputs and temporary test files are ignored by .gitignore

### Build and Test Commands
```bash
# Build debug version
cd /home/conrad/src/SupraFit/debug && make -j4 2>&1 | tail -20

# Build release version
cd /home/conrad/src/SupraFit/release && make -j4

# Run all tests
make test

# Run specific test
./bin/linux/test_integration

# Check build status
make --build debug 2>&1 | tail -20
```
