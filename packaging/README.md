# Упаковка

Версия берётся из `CMakeLists.txt` (`project(ghew VERSION …)`).

Продукт: **GHEW** (Git Hub Empery World) by EmperyWorld.

## Полный релиз (bump + RPM + AppImage)

```bash
./packaging/release.sh              # patch +1, затем оба артефакта
VERSION=0.3.1 ./packaging/release.sh  # явная версия
```

## RPM (Fedora / dnf)

```bash
./packaging/build-rpm.sh
sudo dnf install ~/rpmbuild/RPMS/x86_64/ghew-<ver>-*.rpm
```

Ставить только основной `ghew-*.rpm`, не `debuginfo` / `debugsource`.

Зависимости runtime: `kf6-kirigami`, `qt6-qtdeclarative`, `kf6-ki18n`, `git`.

Пакет `Obsoletes: git-lab` — старый `git-lab` заменяется при установке.

## AppImage

```bash
./packaging/appimage/build.sh
```

Результат: `packaging/out/GHEW-<ver>-<arch>.AppImage` (+ симлинк `GHEW-<arch>.AppImage`).

На системе с KDE проще ставить RPM — тема Breeze и QML-модули берутся из дистрибутива.

## Desktop

`ghew.desktop` — Categories `Development;RevisionControl;Education;` для меню KDE. Icon: логотип EmperyWorld.
