# Sprint 17

| # | Улучшение | Статус |
|---|-----------|--------|
| S17-1 | Команды `worktree_remove`, `worktree_prune` | done |
| S17-2 | UI worktrees: +wt / prune / open / remove / force | done |
| S17-3 | Индикатор prunable (P) на worktree | done |
| S17-4 | Коммиты: ПКМ / колесо — копировать short hash | done |

```bash
cmake --build build -j
./build/bin/git-lab-tests
./build/bin/git-lab --self-check
./build/bin/git-lab --integration-test
bash scripts/smoke-test.sh
qmllint-qt6 -I build -I /usr/lib64/qt6/qml qml/*.qml
```
