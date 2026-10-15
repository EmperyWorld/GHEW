# GHEW

Нативное Linux-приложение для работы с Git: полный каталог команд, расширенные пояснения и обязательное превью последствий до выполнения. Ориентир — **Fedora KDE Plasma**.

## Что умеет

- Выбор локального git-репозитория (папка с `.git`); все команды идут из его корня
- Каталог команд с группами, параметрами и уровнем опасности (`safe` / `mutates` / `destructive`)
- Расширенные пояснения на русском: зачем команда, когда применять, риски, связи с другими
- Превью: точная командная строка + что изменится в репозитории
- Статус репозитория: незакоммиченные файлы, diff --stat, локальные/remote коммиты, ahead/behind
- Diff по клику на файл, цветной diff, blame/show коммита, stash-список
- Быстрые действия (status/diff/add/commit/pull/push…) и избранные команды
- Поиск по каталогу команд, недавние репозитории, автообновление статуса
- Копирование cmdline / diff / вывода в буфер
- Рекомендации следующих действий (правила + опционально LLM / SSE-стриминг) и «спросить наставника»
- Hunk stage/unstage/discard + экспорт `.patch`, сравнение двух коммитов, reflog, теги, remotes, мастера и учебный репозиторий
- Welcome: открыть / **создать (`git init`)** / клонировать; палитра (Ctrl+P), ветка (Ctrl+B), тег (Ctrl+T)
- Запуск с путём: `ghew /path/to/repo` или открытие папки из файлового менеджера
- Слежение за FS, фильтр коммитов, stash show / cherry-pick из Inspect
- Повтор команд из истории сессии, аудит JSONL, экспорт отчёта в Markdown
- Настройки LLM (URL/модель/ключ) и горячие клавиши (Ctrl+O, F5, Ctrl+L, Ctrl+K, Ctrl+P, Ctrl+B)
- Выполнение через системный `git` (без shell), отмена длинных операций, вывод как в терминале

## Стек

| Слой | Технология |
|------|------------|
| UI | Qt6 Quick (QML) + Kirigami 6 |
| Логика | C++20 |
| Сборка | CMake, ECM, Ninja |
| Git | системный бинарь `git` через `QProcess` |
| Поставка | RPM (`dnf`) и AppImage |

## Структура репозитория

```
ghscr/
├── README.md
├── docs/                 # документация проекта
├── CMakeLists.txt
├── src/                  # C++ (контроллер, git, каталог, превью)
├── qml/                  # Kirigami UI
├── data/commands/        # JSON-описания команд
├── packaging/            # RPM, AppImage, .desktop
└── resources/            # иконки и прочие ресурсы
```

Подробнее: [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md), [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md), [docs/COMMAND_CATALOG.md](docs/COMMAND_CATALOG.md).

## Зависимости (Fedora / KDE)

Сборка и runtime:

```bash
sudo dnf install cmake ninja-build gcc-c++ extra-cmake-modules \
  qt6-qtbase-devel qt6-qtdeclarative-devel \
  kf6-kirigami-devel kf6-kirigami kf6-ki18n-devel git
```

На KDE Plasma стиль `org.kde.desktop` подхватит Breeze автоматически.

## Сборка

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/bin/ghew
```

## Тесты и приёмка

```bash
./build/bin/ghew-tests              # UNIT_OK
./build/bin/ghew --self-check       # SELF_CHECK_OK
./build/bin/ghew --integration-test # INTEGRATION_OK (lesson + hunks)
bash scripts/smoke-test.sh             # SMOKE_OK
bash scripts/ci-packaging-smoke.sh     # PACKAGING_SMOKE_OK
```

План приёмки (60 пунктов): [docs/ACCEPTANCE_IMPROVEMENTS.md](docs/ACCEPTANCE_IMPROVEMENTS.md).  
Sprint 4–17: [SPRINT4](docs/SPRINT4.md) · [5](docs/SPRINT5.md) · [6](docs/SPRINT6.md) · [7](docs/SPRINT7.md) · [8](docs/SPRINT8.md) · [9](docs/SPRINT9.md) · [10](docs/SPRINT10.md) · [11](docs/SPRINT11.md) · [12](docs/SPRINT12.md) · [13](docs/SPRINT13.md) · [14](docs/SPRINT14.md) · [15](docs/SPRINT15.md) · [16](docs/SPRINT16.md) · [17](docs/SPRINT17.md).

## Диагностика IDE (QML)

Если в Problems сотни `Unqualified access` / `Member i18n not found`:

1. Убедись, что проект собран (`build/` существует).
2. В корне есть [`.qmlls.ini`](.qmlls.ini) с `importPaths=…/build:…/qt6/qml`.
3. Перезапусти QML Language Server / окно Cursor (`Developer: Reload Window`).

Проверка вручную:

```bash
qmllint-qt6 -I build -I /usr/lib64/qt6/qml qml/*.qml
```

Должно быть 0 warnings.

## Установка

Версия в `CMakeLists.txt`. После доработок: bump + оба артефакта —

```bash
./packaging/release.sh
```

**RPM:** `~/rpmbuild/RPMS/x86_64/ghew-<ver>-*.rpm` (только основной пакет, не debuginfo).  
**AppImage:** `packaging/out/Ghew-<ver>-x86_64.AppImage`.

См. [packaging/README.md](packaging/README.md).

## LLM (рекомендации и советы)

Тот же OpenAI-совместимый контракт, что в `local-agent` (`/v1/models`, `/v1/chat/completions`):

```bash
export GIT_LAB_LLM_BASE_URL=http://127.0.0.1:8080/v1   # или LLM_BASE_URL
export GIT_LAB_LLM_MODEL=gemma-4-26b.gguf              # или LLM_MODEL
export GIT_LAB_LLM_API_KEY=                            # или LLM_API_KEY (только localhost/LAN)
```

Правила рекомендаций работают без LLM. Кнопка «Проверить LLM» / «Обогатить LLM» и поле «Спросить» — во вкладке «Советы».

## Безопасность

- UI локальный; LLM-клиент ходит только на loopback / RFC1918 (как в local-agent)
- Разрешён только запуск `git` с argv из каталога + валидированных полей формы
- Опасные операции требуют отдельного подтверждения после превью

## Лицензия

MIT (см. [LICENSE](LICENSE)). Сторонние компоненты сохраняют свои лицензии;
см. [NOTICE](NOTICE).

Публичные исходники: [EmperyWorld/GHEW](https://github.com/EmperyWorld/GHEW).
