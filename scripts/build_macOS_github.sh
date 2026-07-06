#!/bin/bash
set -ex

# Submodules are already provided at the pinned commits by actions/checkout
# (submodules: recursive) — the old per-submodule "checkout master" loop is removed
# (it ignored the pins and broke on submodules without a local master branch).

mkdir -p release
cd release
cmake -DCMAKE_BUILD_TYPE=Release -Dnoto_font=true -D_Theme=false  ..
# Build only the deployed executables (not the test suite); -j2 for stable peak RAM.
make -j2 suprafit suprafit_cli
cd bin/macOS
macdeployqt  suprafit.app -dmg
