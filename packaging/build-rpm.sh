#!/usr/bin/env bash
# Build a binary RPM from the current source tree (Fedora / rpmbuild).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
VERSION="$(rg -o 'project\(ghew VERSION ([0-9.]+)' -r '$1' "$ROOT/CMakeLists.txt" | head -1)"
VERSION="${VERSION:-0.1.0}"
NAME="ghew"
TARBALL="${NAME}-${VERSION}.tar.gz"
RPMBUILD="${RPMBUILD:-$HOME/rpmbuild}"

mkdir -p "$RPMBUILD"/{BUILD,RPMS,SOURCES,SPECS,SRPMS}

STAGE="$(mktemp -d)"
trap 'rm -rf "$STAGE"' EXIT
mkdir -p "$STAGE/${NAME}-${VERSION}"
rsync -a --exclude build --exclude build-\* --exclude .git --exclude packaging/out \
  "$ROOT/" "$STAGE/${NAME}-${VERSION}/"

tar -C "$STAGE" -czf "$RPMBUILD/SOURCES/$TARBALL" "${NAME}-${VERSION}"

SPEC_OUT="$RPMBUILD/SPECS/${NAME}.spec"
cat > "$SPEC_OUT" <<EOF
Name:           ${NAME}
Version:        ${VERSION}
Release:        1%{?dist}
Summary:        GHEW — Git Hub Empery World (EmperyWorld)
License:        MIT
URL:            https://emperyworld.local/ghew
Source0:        ${TARBALL}
Obsoletes:      git-lab < 0.3.0
Provides:       git-lab = %{version}-%{release}

BuildRequires:  cmake >= 3.22
BuildRequires:  ninja-build
BuildRequires:  gcc-c++
BuildRequires:  extra-cmake-modules
BuildRequires:  qt6-qtbase-devel
BuildRequires:  qt6-qtdeclarative-devel
BuildRequires:  kf6-kirigami-devel
BuildRequires:  kf6-ki18n-devel
BuildRequires:  git

Requires:       kf6-kirigami
Requires:       qt6-qtdeclarative
Requires:       kf6-ki18n
Requires:       git
Recommends:     gh

%description
GHEW (Git Hub Empery World) by EmperyWorld — native Linux (KDE/Kirigami)
Git workspace with Russian explanations, mandatory previews, and GitHub
repository setup through the GitHub CLI.

%prep
%autosetup -n ${NAME}-${VERSION}

%build
cmake -S . -B build -G Ninja \\
  -DCMAKE_BUILD_TYPE=Release \\
  -DCMAKE_INSTALL_PREFIX=%{_prefix}
cmake --build build

%install
DESTDIR=%{buildroot} cmake --install build

%files
%license LICENSE
%doc README.md docs/
%{_bindir}/ghew
%{_datadir}/ghew/
%{_datadir}/applications/ghew.desktop
%{_datadir}/icons/hicolor/256x256/apps/ghew.png
%{_datadir}/icons/hicolor/128x128/apps/ghew.png

%changelog
* Wed Oct 07 2026 EmperyWorld <dev@emperyworld.local> - ${VERSION}-1
- Rebrand to GHEW (Git Hub Empery World)
EOF

rpmbuild -bb "$SPEC_OUT"
echo "RPM(s):"
find "$RPMBUILD/RPMS" -name "${NAME}-*.rpm" -printf '%p\n'
