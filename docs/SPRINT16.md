# Sprint 16

| # | Улучшение | Статус |
|---|-----------|--------|
| S16-1 | `branch_set_upstream` / `branch_unset_upstream` | done |
| S16-2 | `branch_rename` с опциональным old | done |
| S16-3 | BranchDialog: rename / track / delete; prefill текущей | done |
| S16-4 | Список веток: ЛКМ switch/copy, ПКМ track, Shift+ПКМ delete, Ctrl+ПКМ rename | done |
| S16-5 | Advisor предлагает track при отсутствии upstream | done |

```bash
cmake --build build -j
./build/bin/git-lab-tests
./build/bin/git-lab --self-check
./build/bin/git-lab --integration-test
bash scripts/smoke-test.sh
qmllint-qt6 -I build -I /usr/lib64/qt6/qml qml/*.qml
```
