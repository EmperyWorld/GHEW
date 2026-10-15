#!/usr/bin/env bash
# Bump patch version (or set VERSION=x.y.z) and build RPM + AppImage.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
CMAKE="$ROOT/CMakeLists.txt"
CUR="$(rg -o 'project\(ghew VERSION ([0-9.]+)' -r '$1' "$CMAKE" | head -1)"
CUR="${CUR:-0.0.0}"

if [[ -n "${VERSION:-}" ]]; then
  NEW="$VERSION"
else
  IFS=. read -r MAJ MIN PAT <<<"$CUR"
  PAT="${PAT:-0}"
  NEW="${MAJ}.${MIN}.$((PAT + 1))"
fi

echo "==> version ${CUR} → ${NEW}"
sed -i -E "s/project\(ghew VERSION [0-9.]+/project(ghew VERSION ${NEW}/" "$CMAKE"
if [[ -f "$ROOT/packaging/ghew.spec" ]]; then
  sed -i -E "s/^Version:.*/Version:        ${NEW}/" "$ROOT/packaging/ghew.spec"
fi

"$ROOT/packaging/build-rpm.sh"
"$ROOT/packaging/appimage/build.sh"

echo "==> release ${NEW} done"
find "$HOME/rpmbuild/RPMS" -name "ghew-${NEW}-*.rpm" ! -name '*debug*' -printf 'RPM %p\n' 2>/dev/null || true
ls -lh "$ROOT/packaging/out/GHEW-${NEW}"-*.AppImage 2>/dev/null || true
