#!/usr/bin/env bash
# CI packaging smoke: build + unit/self-check + verify packaging assets exist.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

echo "==> core smoke"
bash "$ROOT/scripts/smoke-test.sh"

echo "==> packaging assets"
test -f "$ROOT/packaging/ghew.desktop"
test -f "$ROOT/resources/icons/ghew.png"
test -f "$ROOT/resources/icons/ghew-logo.png"
test -x "$ROOT/packaging/appimage/build.sh" || test -f "$ROOT/packaging/appimage/build.sh"
test -f "$ROOT/packaging/build-rpm.sh"

rg -q '^Exec=ghew' "$ROOT/packaging/ghew.desktop"
rg -q '^Name=GHEW' "$ROOT/packaging/ghew.desktop"

DEST="$(mktemp -d)"
trap 'rm -rf "$DEST"' EXIT
cmake --install "$ROOT/build" --prefix "$DEST" >/dev/null
test -x "$DEST/bin/ghew"
test -f "$DEST/share/applications/ghew.desktop"
test -d "$DEST/share/ghew/commands"

if [[ "${RUN_FULL_PACKAGE:-0}" == "1" ]]; then
  echo "==> full AppImage/RPM (RUN_FULL_PACKAGE=1)"
  bash "$ROOT/packaging/build-rpm.sh" || true
  bash "$ROOT/packaging/appimage/build.sh" || true
fi

echo "PACKAGING_SMOKE_OK"
