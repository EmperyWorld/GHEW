# Sprint 6

| # | Улучшение | Статус |
|---|-----------|--------|
| S6-1 | Discard selected hunks (`git apply -R` в рабочей копии) | done |
| S6-2 | Экспорт выбранных hunks в `.patch` | done |
| S6-3 | TagListModel + секция тегов в статус-панели | done |
| S6-4 | Фильтр коммитов (subject / author / hash) | done |
| S6-5 | Диалог ветки (клик / Ctrl+B → create+switch / switch) | done |
| S6-6 | Persist окна и вкладки workspace | done |
| S6-7 | Integration: unstage / discard / export patch | done |

```bash
./build/bin/git-lab-tests
./build/bin/git-lab --self-check
./build/bin/git-lab --integration-test
bash scripts/smoke-test.sh
qmllint-qt6 -I build qml/*.qml
```
