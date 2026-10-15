# Sprint 7

| # | Улучшение | Статус |
|---|-----------|--------|
| S7-1 | Stash show в Inspect (клик); ПКМ → apply | done |
| S7-2 | RemoteListModel + секция remotes (копирование URL) | done |
| S7-3 | Cherry-pick / revert / compare из Inspect коммита | done |
| S7-4 | Поиск по строкам в InspectPanel | done |
| S7-5 | Копировать абсолютный path / открыть папку файла | done |
| S7-6 | Quick action «amend» | done |
| S7-7 | stash_apply с опциональным ref | done |

```bash
./build/bin/git-lab-tests
./build/bin/git-lab --self-check
./build/bin/git-lab --integration-test
bash scripts/smoke-test.sh
qmllint-qt6 -I build qml/*.qml
```
