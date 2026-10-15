#!/usr/bin/env bash
# Build AppDir and AppImage. Downloads linuxdeploy + appimagetool if missing.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$ROOT/build-release}"
OUT_DIR="$ROOT/packaging/out"
APPDIR="$OUT_DIR/GHEW.AppDir"
ARCH="$(uname -m)"
TOOLS_DIR="$OUT_DIR/tools"
VERSION="$(rg -o 'project\(ghew VERSION ([0-9.]+)' -r '$1' "$ROOT/CMakeLists.txt" | head -1)"
VERSION="${VERSION:-0.0.0}"
mkdir -p "$TOOLS_DIR" "$OUT_DIR"

download() {
  local url="$1" dest="$2"
  if [[ -x "$dest" ]]; then
    return 0
  fi
  echo "==> downloading $(basename "$dest")"
  curl -fsSL -o "$dest" "$url"
  chmod +x "$dest"
}

if [[ "$ARCH" == "x86_64" ]]; then
  download \
    "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage" \
    "$TOOLS_DIR/linuxdeploy"
  download \
    "https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage" \
    "$TOOLS_DIR/appimagetool"
  download \
    "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage" \
    "$TOOLS_DIR/linuxdeploy-plugin-qt"
fi

LINUXDEPLOY="${LINUXDEPLOY:-$TOOLS_DIR/linuxdeploy}"
APPIMAGETOOL="${APPIMAGETOOL:-$TOOLS_DIR/appimagetool}"
export PATH="$TOOLS_DIR:$PATH"

echo "==> Release build"
cmake -S "$ROOT" -B "$BUILD_DIR" -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR"

rm -rf "$APPDIR"
mkdir -p \
  "$APPDIR/usr/bin" \
  "$APPDIR/usr/share/ghew" \
  "$APPDIR/usr/share/applications" \
  "$APPDIR/usr/share/icons/hicolor/256x256/apps"

cp "$BUILD_DIR/bin/ghew" "$APPDIR/usr/bin/"
cp -a "$ROOT/data/." "$APPDIR/usr/share/ghew/"
cp "$ROOT/packaging/ghew.desktop" "$APPDIR/usr/share/applications/"
cp "$ROOT/resources/icons/ghew.png" "$APPDIR/usr/share/icons/hicolor/256x256/apps/ghew.png"
cp "$APPDIR/usr/share/applications/ghew.desktop" "$APPDIR/ghew.desktop"
cp "$APPDIR/usr/share/icons/hicolor/256x256/apps/ghew.png" "$APPDIR/ghew.png"

cat > "$APPDIR/AppRun" <<'EOF'
#!/usr/bin/env bash
HERE="$(dirname "$(readlink -f "$0")")"
export GHEW_DATA_DIR="${GHEW_DATA_DIR:-$HERE/usr/share/ghew}"
export QML2_IMPORT_PATH="$HERE/usr/qml:${QML2_IMPORT_PATH:-/usr/lib64/qt6/qml:/usr/lib/qt6/qml}"
export QT_PLUGIN_PATH="$HERE/usr/plugins:${QT_PLUGIN_PATH:-}"
exec "$HERE/usr/bin/ghew" "$@"
EOF
chmod +x "$APPDIR/AppRun"

if [[ ! -x "$LINUXDEPLOY" ]]; then
  echo "ERROR: linuxdeploy not available for arch=$ARCH"
  echo "AppDir prepared at $APPDIR"
  exit 1
fi

echo "==> bundling with linuxdeploy"
export NO_STRIP=1
export DISABLE_COPYRIGHT_FILES_DEPLOYMENT=1
export QML_SOURCES_PATHS="$ROOT/qml"
export EXTRA_QT_PLUGINS="${EXTRA_QT_PLUGINS:-platforms;wayland;xcb}"
"$LINUXDEPLOY" --appdir "$APPDIR" \
  --executable "$APPDIR/usr/bin/ghew" \
  --desktop-file "$APPDIR/ghew.desktop" \
  --icon-file "$APPDIR/ghew.png" \
  --plugin qt

IMAGE="$OUT_DIR/GHEW-${VERSION}-${ARCH}.AppImage"
RUNTIME_FILE="${RUNTIME_FILE:-$TOOLS_DIR/runtime-x86_64}"
if [[ ! -s "$RUNTIME_FILE" && "$ARCH" == "x86_64" ]]; then
  echo "==> caching AppImage type2 runtime"
  curl -fsSL -o "$RUNTIME_FILE" \
    "https://github.com/AppImage/type2-runtime/releases/download/continuous/runtime-x86_64" || true
fi
if [[ -s "$RUNTIME_FILE" ]]; then
  "$APPIMAGETOOL" --runtime-file "$RUNTIME_FILE" "$APPDIR" "$IMAGE"
else
  "$APPIMAGETOOL" "$APPDIR" "$IMAGE"
fi
chmod +x "$IMAGE"
ln -sfn "$(basename "$IMAGE")" "$OUT_DIR/GHEW-${ARCH}.AppImage"
echo "Built $IMAGE"
ls -lh "$IMAGE"
