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
# -codesign=- re-signs the bundle ad-hoc after deployment. Without it the app keeps only the
# linker-signed signature, which seals no resources - while macdeployqt copies SupraFit.icns into
# Contents/Resources. A bundle in that state fails `codesign --verify --deep` with "code has no
# resources but signature indicates they must be present", which blocked the artifact upload.
macdeployqt  suprafit.app -codesign=- -dmg
