# Sprint 10

| # | Улучшение | Статус |
|---|-----------|--------|
| S10-1 | SubmoduleListModel + секция статуса | done |
| S10-2 | HookListModel (hooks dir, active/sample) | done |
| S10-3 | Clean dry-run в Inspect + переход к clean -fd | done |
| S10-4 | `git check-attr -a` + LFS-подсказка | done |
| S10-5 | j/k (↑/↓) навигация по dirty-файлам | done |

```bash
./build/bin/git-lab-tests
./build/bin/git-lab --self-check
./build/bin/git-lab --integration-test
bash scripts/smoke-test.sh
qmllint-qt6 -I build qml/*.qml
```
