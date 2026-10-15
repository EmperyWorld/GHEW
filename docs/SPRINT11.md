# Sprint 11

| # | Улучшение | Статус |
|---|-----------|--------|
| S11-1 | CLI: `git-lab /path/to/repo` (и `file://`) открывает репозиторий | done |
| S11-2 | Desktop: `Exec=git-lab %F` + `MimeType=inode/directory` (открыть папку из Dolphin) | done |
| S11-3 | Persist ширина левого SplitView (`sidebarWidth`) | done |
| S11-4 | Свежий RPM после UI-полировки | done |

```bash
./build/bin/git-lab .
./build/bin/git-lab --self-check
bash scripts/smoke-test.sh
./packaging/build-rpm.sh
```
