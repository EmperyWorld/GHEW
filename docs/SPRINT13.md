# Sprint 13

| # | Улучшение | Статус |
|---|-----------|--------|
| S13-1 | DiffPanel: группа действий при конфликте (ours / theirs / решено / редактор) | done |
| S13-2 | StatusPanel: полоска действий + автофильтр `!` при появлении конфликтов | done |
| S13-3 | `stash_drop` / `stash_pop_ref`; ПКМ apply, Shift+ПКМ drop с ref | done |
| S13-4 | Integration: sandbox merge-конфликта → `operationKind=merge` | done |

```bash
cmake --build build -j
./build/bin/git-lab-tests
./build/bin/git-lab --self-check
./build/bin/git-lab --integration-test
bash scripts/smoke-test.sh
qmllint-qt6 -I build -I /usr/lib64/qt6/qml qml/*.qml
```
