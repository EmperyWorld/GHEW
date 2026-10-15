Name:           ghew
Version:        0.10.4
Release:        1%{?dist}
Summary:        GHEW — Git Hub Empery World (EmperyWorld)
License:        MIT
URL:            https://emperyworld.local/ghew
# Prefer: ./packaging/build-rpm.sh which fills Source0 and concrete scriptlets.
Source0:        ghew-%{version}.tar.gz
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
%autosetup

%build
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
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
* Fri Oct 09 2026 EmperyWorld <dev@emperyworld.local> - 0.10.4-1
- Production mentor path: free-text→gh_file, config_set_user, wizard run step, offline RUN
* Fri Oct 09 2026 EmperyWorld <dev@emperyworld.local> - 0.10.3-1
- Mentor free-text reply; ask_user without chips; integration covers dress write + RUN gh_file
* Fri Oct 09 2026 EmperyWorld <dev@emperyworld.local> - 0.10.2-1
- Fix advisor dress/config apply: template RUN writes file, snapshot tracks dress.*, no empty writes
* Fri Oct 09 2026 EmperyWorld <dev@emperyworld.local> - 0.10.1-1
- Fix catalog recent chips and command list overflowing past the sidebar splitter
* Fri Oct 09 2026 EmperyWorld <dev@emperyworld.local> - 0.10.0-1
- UI shell refactor: ScrollPane/ChipFlow/SoftBar, Theme.uiScale tokens, slim workspace chrome
* Wed Oct 07 2026 EmperyWorld <dev@emperyworld.local> - 0.3.0-1
- Rebrand to GHEW (Git Hub Empery World)
