# Разработка

## Требования

- Fedora 40+ (проверено на fc44) или совместимый RPM-дистрибутив
- KDE Plasma желателен для визуальной проверки (Kirigami подхватит Breeze)
- CMake ≥ 3.22, Ninja, GCC с C++20
- Qt 6 (Base + Declarative), KF6 Kirigami, ECM, git

## Установка зависимостей

```bash
sudo dnf install cmake ninja-build gcc-c++ extra-cmake-modules \
  qt6-qtbase-devel qt6-qtdeclarative-devel \
  kf6-kirigami-devel kf6-kirigami git
```

## Конфигурация и сборка

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

cmake --build build
./build/bin/git-lab
```

Переменная для данных при запуске из build-дерева выставляется CMake/`main` автоматически; при необходимости:

```bash
export GIT_LAB_DATA_DIR="$PWD/data"
./build/bin/git-lab
```

## Соглашения по коду

- C++20, без исключений в горячем пути git-вызовов где возможно — ошибки через `Result`/`QString`
- UI-строки — на русском (`tr` / qsTr), комментарии в коде — по делу, на английском или русском единообразно в файле
- Новые команды — только через JSON-каталог + при необходимости хук в `PreviewEngine`
- Не добавлять свободный `sh -c` / произвольный бинарь

## Как добавить команду

1. Описать JSON в `data/commands/<group>.json` (см. [COMMAND_CATALOG.md](COMMAND_CATALOG.md)).
2. Если нужен особый превью-текст — добавить ветку в `PreviewEngine` по `id`.
3. Пересобрать не обязательно для правок только текста в JSON, если данные читаются с диска при старте; при встраивании в qrc — нужна пересборка ресурсов.

## Тестирование вручную

1. Открыть тестовый git-репозиторий (можно временный `git init /tmp/git-lab-sandbox`).
2. Для safe-команд (`status`, `log`) — превью и execute без confirm.
3. Для mutates (`add`, `commit`) — проверить narrative и реальный эффект.
4. Для destructive (`reset --hard`, `clean -fd`) — убедиться, что без второго подтверждения execute недоступен.

## Упаковка (кратко)

См. [packaging/README.md](../packaging/README.md).
