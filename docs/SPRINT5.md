# Sprint 5

| # | Улучшение | Статус |
|---|-----------|--------|
| S5-1 | QFileSystemWatcher + debounce refresh (.git/HEAD, index, дерево) | done |
| S5-2 | Command palette (Ctrl+P) с недавними командами | done |
| S5-3 | Hunk unstage (`git apply --cached -R`) + режим staged/unstaged | done |
| S5-4 | Clone remote с Welcome | done |
| S5-5 | Подсветка conflict markers в Diff | done |
| S5-6 | RecentCommandsModel (persist) | done |
| S5-7 | Настройка «следить за FS» | done |

```bash
./build/bin/git-lab-tests
./build/bin/git-lab --self-check
./build/bin/git-lab --integration-test
bash scripts/smoke-test.sh
qmllint-qt6 -I build qml/*.qml
```
