#!/usr/bin/env bash
# Re-copy upstream/cd-coop-3446 from GitHub at a given commit (default: latest). Review the diff, then commit it alone.
set -euo pipefail
cd "$(dirname "$0")/.."
REF=${1:-HEAD}
TMP=$(mktemp -d)
git clone -q https://github.com/nouzert1/cd-coop-3446.git "$TMP/src"
git -C "$TMP/src" checkout -q "$REF"
rm -rf upstream/cd-coop-3446 && mkdir -p upstream/cd-coop-3446
git -C "$TMP/src" archive HEAD | tar -x -C upstream/cd-coop-3446
rm -f upstream/cd-coop-3446/third_party/ultimate-asi-loader/*/*.dll
echo "Upstream now at $(git -C "$TMP/src" log -1 --format='%h %ad %s')"
git status --short upstream | head -40
rm -rf "$TMP"
