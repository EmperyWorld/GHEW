# Sprint 12

| # | Улучшение | Статус |
|---|-----------|--------|
| S12-1 | `RepoContext.operationKind` (merge / rebase / cherry-pick / revert) | done |
| S12-2 | `RepoContext.conflictCount` из porcelain + JSON snapshot | done |
| S12-3 | Каталог: cherry-pick/revert `--continue` / `--abort` | done |
| S12-4 | Баннер + QuickActionBar continue/abort; чип `!` в шапке | done |
| S12-5 | Advisor учитывает вид операции | done |

```bash
cmake --build build -j
./build/bin/git-lab-tests
./build/bin/git-lab --self-check
./build/bin/git-lab --integration-test
bash scripts/smoke-test.sh
qmllint-qt6 -I build -I /usr/lib64/qt6/qml qml/*.qml
```
