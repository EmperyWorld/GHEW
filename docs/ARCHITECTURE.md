# Архитектура Git Lab

## Обзор

Один процесс: QML/Kirigami рисует UI, C++ держит состояние репозитория, каталог команд и запуск `git`.

```
┌─────────────────────────────────────────────────────────┐
│  Kirigami ApplicationWindow (qml/)                      │
│  toolbar: repo path · branch · status                   │
│  sidebar: группы команд                                 │
│  center: форма параметров                               │
│  right/bottom: Пояснение | Превью | Вывод               │
└──────────────────────┬──────────────────────────────────┘
                       │ QObject properties / invokables
┌──────────────────────▼──────────────────────────────────┐
│  AppController                                          │
│  ├── RepoContext      — путь, toplevel, branch, status  │
│  ├── CommandCatalog   — JSON → модели для UI            │
│  ├── PreviewEngine    — cmdline + narrative + impact    │
│  └── GitRunner        — QProcess, cwd=repoRoot          │
└──────────────────────┬──────────────────────────────────┘
                       │
                       ▼
                   git (CLI)
```

## Модули C++ (`src/`)

| Файл | Назначение |
|------|------------|
| `main.cpp` | QGuiApplication / QApplication, движок QML, регистрация типов |
| `app/AppController` | Фасад для QML: выбрать репо, выбрать команду, preview, execute |
| `core/RepoContext` | Валидация `.git`, `rev-parse`, кэш short status |
| `core/GitRunner` | Единая точка запуска: timeout, лимит вывода, только `git` |
| `core/CommandCatalog` | Загрузка `data/commands/*.json`, поиск по id/группе |
| `core/PreviewEngine` | Сборка argv, dry-run где возможно, текст «что произойдёт» |
| `core/CommandSpec` | Структуры: param, danger level, metadata |

## Поток «превью → выполнение»

1. Пользователь заполняет параметры команды в UI.
2. `AppController::preview(commandId, args)` → `PreviewEngine`:
   - строит argv по спецификации команды;
   - при необходимости дергает `GitRunner` (`status`, `diff --stat`, `push --dry-run`, …);
   - возвращает `PreviewResult` (cmdline, narrative, danger, confirmToken).
3. UI показывает превью; для `destructive` — PromptDialog.
4. `AppController::execute(confirmToken)` проверяет токен и вызывает `GitRunner` с тем же argv.
5. stdout/stderr/exit code → панель «Вывод»; `RepoContext` обновляет status.

## Рабочий каталог

После выбора папки `RepoContext` нормализует путь через `git rev-parse --show-toplevel`. Все последующие вызовы `GitRunner` используют этот путь как `QProcess::setWorkingDirectory`.

## Данные команд

JSON в `data/commands/` (по группам или один файл на группу). Схема — в [COMMAND_CATALOG.md](COMMAND_CATALOG.md). Пояснения хранятся в JSON (markdown), чтобы править тексты без пересборки логики превью-хуков.

## Потоки и UI

`GitRunner` выполняет процессы асинхронно (`QProcess` + сигналы). Долгие операции не блокируют event loop. Результаты приходят в UI через сигналы `AppController`.

## Упаковка

- RPM ставит бинарь в `/usr/bin/git-lab`, QML/данные в `/usr/share/git-lab/`, `.desktop` в `/usr/share/applications/`.
- AppImage кладёт тот же layout внутрь AppDir.

Путь к `data/commands` ищется относительно бинаря или через `GIT_LAB_DATA_DIR`.
