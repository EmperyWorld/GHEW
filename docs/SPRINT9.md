# Sprint 9

| # | Улучшение | Статус |
|---|-----------|--------|
| S9-1 | WorktreeListModel + секция (клик → open) | done |
| S9-2 | Bisect: детект BISECT_START, мастер, quick good/bad/reset | done |
| S9-3 | Quick unstage all (`restore --staged .`) | done |
| S9-4 | Счётчики `+` / `−` строк в DiffPanel | done |
| S9-5 | Открыть файл во внешнем редакторе | done |
| S9-6 | Клик по upstream / ahead / behind → fetch/push/pull | done |

```bash
./build/bin/git-lab-tests
./build/bin/git-lab --self-check
./build/bin/git-lab --integration-test
bash scripts/smoke-test.sh
qmllint-qt6 -I build qml/*.qml
```
