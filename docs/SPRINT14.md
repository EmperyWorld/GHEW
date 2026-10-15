# Sprint 14

| # | Улучшение | Статус |
|---|-----------|--------|
| S14-1 | Команды `pull_rebase`, `fetch_prune` | done |
| S14-2 | QuickActionBar: prune / rebase↓ / push -u; Advisor для diverged | done |
| S14-3 | Шапка: `diverged`, ↓ ПКМ = pull --rebase | done |
| S14-4 | Терминал в корне репо; ПКМ по ветке — копировать; клик по имени репо — папка | done |
| S14-5 | Мастер sync → pull --rebase / push -u | done |

```bash
cmake --build build -j
./build/bin/git-lab-tests
./build/bin/git-lab --self-check
./build/bin/git-lab --integration-test
bash scripts/smoke-test.sh
qmllint-qt6 -I build -I /usr/lib64/qt6/qml qml/*.qml
```
