#!/bin/bash
set -ex

# Submodules are already provided at the pinned commits by actions/checkout
# (submodules: recursive) — the old per-submodule "checkout master" loop is removed
# (it ignored the pins and broke on submodules without a local master branch).

mkdir -p release
cd release
cmake -DCMAKE_BUILD_TYPE=Release -Dnoto_font=true -D_Theme=false  ..
make -j$(sysctl -n hw.ncpu)
cd bin/macOS
macdeployqt  suprafit.app -dmg
