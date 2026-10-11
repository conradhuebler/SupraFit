#!/bin/bash
set -ex

# Use the runner's default (modern) toolchain — the old gcc-9 is not on ubuntu-22.04
# and is too old for the current C++17 codebase.
# Submodules are already provided at the pinned commits by actions/checkout (submodules:
# recursive); the old "checkout master in every submodule" loop both ignored the pins and
# failed on submodules without a local master branch — removed.

mkdir -p release
cd release
#cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -Dnoto_font=true -D_Theme=false -DCMAKE_PREFIX_PATH=$HOME/SupraFit/Qt/5.15.1/gcc_64 ..
cmake -DCMAKE_BUILD_TYPE=Release -Dnoto_font=true -D_Theme=false  ..
# Build only the deployed executables (not the test suite) — less memory pressure and
# faster CI. -j2 keeps peak RAM in check for the heavy exprtk.hpp translation unit.
make -j2 suprafit suprafit_cli
