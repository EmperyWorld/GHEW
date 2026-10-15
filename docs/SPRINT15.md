# Sprint 15

| # | Улучшение | Статус |
|---|-----------|--------|
| S15-1 | `lastCommitSubject` + prefill amend («из HEAD») | done |
| S15-2 | `suggestCommitMessage()` + чип «из файлов» | done |
| S15-3 | Ctrl+Enter выполнить превью; F1 → Настройки | done |
| S15-4 | Ctrl+Shift+T терминал; список клавиш в Settings | done |
| S15-5 | Welcome: ПКМ по недавнему — убрать из списка | done |

```bash
cmake --build build -j
./build/bin/git-lab-tests
./build/bin/git-lab --self-check
./build/bin/git-lab --integration-test
bash scripts/smoke-test.sh
qmllint-qt6 -I build -I /usr/lib64/qt6/qml qml/*.qml
```
