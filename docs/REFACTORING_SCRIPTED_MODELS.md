# Refactoring Plan for Scripted Models

This document outlines a refactoring plan to improve the implementation of scripted models in SupraFit.

## 1. Analysis of Current Implementation

The current implementation of scripted models in SupraFit has the following characteristics:

- **Multiple Interpreter Backends:** The code has support for three different scripting backends: ChaiScript, Duktape, and Python. It also has a partial implementation for `exprtk`. This makes the code complex and difficult to maintain.
- **Redundant Interpreters:** The `QJSEngine` is also used for some calculations, which adds another layer of complexity.
- **Inconsistent API:** The API for interacting with the different interpreters is not consistent. For example, the `CalculateChai()`, `CalculatePython()`, and `CalculateDuktape()` functions all have different implementations.
- **Performance Issues:** The documentation mentions that the ChaiScript implementation is 5-10 times slower than the built-in models. This is likely due to the overhead of the scripting engine.
- **Limited Functionality:** The documentation states that scripted models cannot be used for data with more than one output column (series).
- **Outdated Documentation:** The `docs/ScriptedModels.md` file contains some outdated information and refers to a "changing syntax in progress."

## 2. Refactoring Plan

I propose the following refactoring plan to improve the implementation of scripted models in SupraFit:

### 2.1. Create a Consistent API

- **Goal:** To make it easy to switch between the different scripting backends for performance testing, I recommend creating a consistent API for interacting with them.
- **Tasks:**
    - Create a new `ScriptingEngine` base class that defines a common interface for all scripting engines. This interface should include methods for:
        - Initializing the engine.
        - Setting the script to be executed.
        - Setting the input and output variables.
        - Executing the script.
        - Getting the results.
    - Create concrete `ChaiScriptEngine`, `DuktapeEngine`, `PythonEngine`, and `ExprTkEngine` classes that implement the `ScriptingEngine` interface.
    - Update the `ScriptModel` class to use the `ScriptingEngine` interface instead of directly interacting with the interpreters. This will allow the user to select the scripting engine to be used at runtime.

### 2.2. Improve Performance

- **Goal:** To improve the performance of scripted models, I recommend optimizing the expression evaluation process for each scripting engine.
- **Tasks:**
    - For the `ChaiScriptEngine`, investigate the use of the `eval_file()` function to avoid recompiling the script every time it is executed.
    - For the `DuktapeEngine`, investigate the use of bytecode compilation to improve performance.
    - For the `PythonEngine`, investigate the use of the `Py_CompileString()` function to pre-compile the Python code.
    - For the `ExprTkEngine`, use the library's built-in features for optimizing expressions, such as pre-compiling expressions and using custom functions.

### 2.3. Extend Functionality

- **Goal:** To make scripted models more useful, I recommend extending their functionality to support multiple output columns (series).
- **Tasks:**
    - Update the `ScriptModel` class to handle multiple output columns.
    - Update each of the scripting engine classes to support expressions that return multiple values.

### 2.4. Update Documentation

- **Goal:** To ensure that the documentation is accurate and up-to-date, I recommend updating the `docs/ScriptedModels.md` file.
- **Tasks:**
    - Add a new section that describes the different scripting engines that are available and how to choose between them.
    - Provide clear examples of how to create and use scripted models with each of the different scripting engines.