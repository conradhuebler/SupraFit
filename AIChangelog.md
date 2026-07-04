# AI-assisted Changelog

One line per significant AI-assisted improvement (newest first).

- 2026-07-04: Split god-object `core/analyse.cpp` into a JSON compute TU (`analyse.cpp`) and a string/HTML formatting TU (`analyse_format.cpp`), facade `analyse.h` unchanged — verbatim move, behaviour-preserving (verified via golden `-x` diff).
