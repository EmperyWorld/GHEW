#!/usr/bin/env bash
# End-to-end smoke test against a temporary git repository (no GUI).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
BIN="${BIN:-$ROOT/build/bin/ghew}"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

echo "Building…"
cmake -S "$ROOT" -B "$ROOT/build" -G Ninja -DCMAKE_BUILD_TYPE=Debug >/dev/null
cmake --build "$ROOT/build"

# Self-check CLI
"$BIN" --self-check | tee "$TMP/self-check.txt"
rg -q 'SELF_CHECK_OK' "$TMP/self-check.txt"

# Integration (lesson sandbox + hunk stage + mentor dress/reply path)
"$BIN" --integration-test | tee "$TMP/integration.txt"
rg -q 'INTEGRATION_OK' "$TMP/integration.txt"
rg -q 'OK mentor_dress_snapshot' "$TMP/integration.txt"
rg -q 'OK mentor_ask_free_text' "$TMP/integration.txt"
rg -q 'OK mentor_reply_to_gh_file' "$TMP/integration.txt"
rg -q 'OK mentor_run_gh_file' "$TMP/integration.txt"
rg -q 'OK config_set_user' "$TMP/integration.txt"

# Unit tests
"$ROOT/build/bin/ghew-tests"

# Catalog integrity
python3 - <<'PY'
import json, glob, sys
ids=set(); n=0
for p in glob.glob("data/commands/*.json"):
    d=json.load(open(p))
    for c in d["commands"]:
        assert c["id"] not in ids, c["id"]
        ids.add(c["id"])
        assert c.get("git") or c.get("gh"), c["id"]
        assert c["danger"] in ("safe","mutates","destructive"), c["id"]
        assert c.get("explanation"), c["id"]
        n+=1
print(f"catalog_ok commands={n}")
if n < 60:
    raise SystemExit("too few commands")
PY

# Temp repo — same operations the app runs via GitRunner
REPO="$TMP/repo"
mkdir -p "$REPO"
git -C "$REPO" init -b main
git -C "$REPO" config user.email "test@ghew.local"
git -C "$REPO" config user.name "GHEW Smoke"
echo "hello" > "$REPO/README.md"
git -C "$REPO" add README.md
git -C "$REPO" commit -m "initial"

run() {
  echo "+ git $*"
  git -C "$REPO" "$@"
}

run status -sb
run log --oneline -n 5
echo "change" >> "$REPO/README.md"
echo "newfile" > "$REPO/EXTRA.txt"
run add README.md
run status --porcelain

# App status dump (dirty tree: staged + untracked)
"$BIN" --dump-status "$REPO" > "$TMP/status-dirty.json"
python3 - "$TMP/status-dirty.json" <<'PY'
import json, sys
j=json.load(open(sys.argv[1]))
assert j["open"] is True
assert j["dirty"] >= 2, j
assert j["staged"] >= 1, j
assert j["untracked"] >= 1, j
assert j["localCommits"] >= 1, j
assert any(f["path"]=="README.md" for f in j["files"]), j["files"]
assert any(f["path"]=="EXTRA.txt" for f in j["files"]), j["files"]
recs=j.get("recommendations") or []
assert len(recs) >= 2, j
ids={r["commandId"] for r in recs}
assert "commit" in ids or "diff_staged" in ids or "add_path" in ids, ids
assert all(r.get("source")=="rule" for r in recs), recs
print("status_dump_ok dirty=%s local=%s remote=%s recs=%d" % (
    j["dirty"], j["localCommits"], j["remoteCommits"], len(recs)))
PY

run commit -m "smoke change"
run add EXTRA.txt
run commit -m "extra"
run log --oneline -n 3
run stash push -u -m "smoke"
run stash list
run stash drop

"$BIN" --dump-status "$REPO" > "$TMP/status-clean.json"
python3 - "$TMP/status-clean.json" <<'PY'
import json, sys
j=json.load(open(sys.argv[1]))
assert j["dirty"] == 0, j
assert j["localCommits"] >= 2, j
print("status_clean_ok local=%(localCommits)s" % j)
PY

# Binary linkage / embed
file "$BIN"
ldd "$BIN" | rg -q 'libQt6Qml'
# Commands embedded in qrc (resource table or explanation text)
if ! strings "$BIN" | rg -q 'ghew/commands|Что делает'; then
  echo "WARN: could not confirm embedded catalog strings (stripped?); checking install data instead"
fi

# Prefix install layout (do NOT launch GUI)
DEST="$TMP/prefix"
cmake --install "$ROOT/build" --prefix "$DEST" >/dev/null
test -x "$DEST/bin/ghew"
test -f "$DEST/share/ghew/commands/inspect.json"
test -f "$DEST/share/applications/ghew.desktop"
test -f "$DEST/share/icons/hicolor/256x256/apps/ghew.png"

# Headless start: must exit 0 after creating window briefly under offscreen, or fail catalog
# Use timeout — offscreen may still run event loop; kill after prove it started.
set +e
QT_QPA_PLATFORM=offscreen timeout 3s "$DEST/bin/ghew" >/tmp/ghew-offscreen.log 2>&1
rc=$?
set -e
# timeout returns 124 when it kills a still-running app — that means UI started OK
if [[ "$rc" -ne 0 && "$rc" -ne 124 ]]; then
  echo "offscreen start failed rc=$rc"
  cat /tmp/ghew-offscreen.log || true
  exit 1
fi
if rg -q 'command catalog is empty' /tmp/ghew-offscreen.log; then
  echo "catalog empty at runtime"
  exit 1
fi

echo "SMOKE_OK"
