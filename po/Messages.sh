#!/usr/bin/env bash
# Extract translatable strings for ki18n domain "git-lab".
# Usage from repo root: bash po/Messages.sh
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
mkdir -p po
# QML via I18n.i18n(...) is not xgettext-friendly; document catalog for translators.
{
  echo '# Git Lab — source of Russian UI strings lives in qml/*.qml (I18n.i18n) and src/**/tr()'
  echo "# Domain: git-lab"
  echo "# Generated: $(date -Iseconds)"
} > po/README.md
# C++ tr() extraction if xgettext available
if command -v xgettext >/dev/null; then
  find src -name '*.cpp' -o -name '*.h' | xgettext -C --kde \
    --from-code=UTF-8 -ktr:1 -ktri18n:1 -o po/git-lab.pot -f - 2>/dev/null || true
fi
echo "po extract done (domain git-lab)"
