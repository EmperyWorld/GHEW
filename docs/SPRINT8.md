# Sprint 8

| # | Улучшение | Статус |
|---|-----------|--------|
| S8-1 | Баннеры bare / detached / submodule в шапке | done |
| S8-2 | Добавление пути в `.gitignore` | done |
| S8-3 | История файла (`git log -- path`) в Inspect | done |
| S8-4 | Поиск по строкам в OutputTerminal | done |
| S8-5 | Диалог создания тега (Ctrl+T) | done |
| S8-6 | Soft reset к inspect-коммиту | done |

```bash
./build/bin/git-lab-tests
./build/bin/git-lab --self-check
./build/bin/git-lab --integration-test
bash scripts/smoke-test.sh
qmllint-qt6 -I build qml/*.qml
```
