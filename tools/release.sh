#!/usr/bin/env bash
# Builds, tests and zips a release from Linux (MinGW cross-build). Usage: tools/release.sh
# The CI workflow (.github/workflows/cdcoop.yml) makes the MSVC build; either works in the game.
set -euo pipefail
cd "$(dirname "$0")/.."
VERSION=$(grep -oP 'project\(CrimsonDesertSeamlessCoop VERSION \K[0-9.]+' CMakeLists.txt)

cmake -S . -B build/linux -G Ninja >/dev/null
cmake --build build/linux
ctest --test-dir build/linux --output-on-failure

cmake -S . -B build/win -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64-x86_64.cmake >/dev/null
cmake --build build/win
x86_64-w64-mingw32-strip build/win/CrimsonDesertCoop.asi

STAGE=dist/stage
rm -rf "$STAGE" && mkdir -p "$STAGE/bin64/cdcoop/config"
cp build/win/CrimsonDesertCoop.asi "$STAGE/bin64/"
cp config/*.toml "$STAGE/bin64/cdcoop/config/"
for f in README.md CREDITS.md CHANGELOG.md LICENSE; do [ -f "$f" ] && cp "$f" "$STAGE/"; done
ZIP="dist/CrimsonDesert-SeamlessCoop-$VERSION.zip"
rm -f "$ZIP"
(cd "$STAGE" && zip -qr "../$(basename "$ZIP")" .)
sha256sum "$ZIP"
echo "Release: $ZIP"
