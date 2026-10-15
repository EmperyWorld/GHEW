# Sprint 4 — после приёмки 60 пунктов

Цель: укрепить надёжность и добавить продуктивные фичи без раздувания UI.

| # | Улучшение | Статус |
|---|-----------|--------|
| S4-1 | Фикс гонки async execute (snapshot commandId/args) | done |
| S4-2 | Headless `--integration-test` (lesson + hunk stage) | done |
| S4-3 | GitHub Actions CI (build/unit/self/integration/smoke) | done |
| S4-4 | Поиск по строкам diff | done |
| S4-5 | Экспорт отчёта сессии в Markdown | done |
| S4-6 | Conflict helpers (merge abort/continue, ours/theirs + advisor) | done |
| S4-7 | Обновление README под текущий функционал | done |

Проверки:

```bash
./build/bin/git-lab-tests
./build/bin/git-lab --self-check
./build/bin/git-lab --integration-test
bash scripts/smoke-test.sh
bash scripts/ci-packaging-smoke.sh
qmllint-qt6 -I build qml/*.qml
```
