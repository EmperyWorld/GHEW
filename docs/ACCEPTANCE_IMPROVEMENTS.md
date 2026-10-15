# Git Lab — план приёмки: 50+ полноценных улучшений

Статусы: `todo` · `doing` · `done` · `partial`

Критерий «полноценно»: не заглушка; есть UI и/или API, проверка через smoke/qmllint/сборку, поведение предсказуемо при ошибках git/LLM.

---

## A. Навигация и UX рабочей области (1–12)

| # | Улучшение | Статус |
|---|-----------|--------|
| 1 | Единая вкладка Inspect для show/blame/stash show | done |
| 2 | Панель быстрых действий (status/diff/add/commit/…) | done |
| 3 | Избранные команды (persist + чипы) | done |
| 4 | Поиск по каталогу команд | done |
| 5 | Недавние репозитории на Welcome | done |
| 6 | Горячие клавиши Ctrl+O / F5 / Ctrl+L | done |
| 7 | Автообновление статуса (настраиваемое) | done |
| 8 | Breadcrumb: repo → branch → tab | done |
| 9 | Splitter: изменяемая ширина сайдбара | done |
| 10 | Сворачивание секций сайдбара (файлы/коммиты/ветки/stash) | done |
| 11 | Фокус поиска команд по Ctrl+K / `/` | done |
| 12 | Busy: индикация busy/LLM/enrich на всю шапку | done |

## B. Статус, diff, история (13–24)

| # | Улучшение | Статус |
|---|-----------|--------|
| 13 | Список dirty-файлов с кодами XY | done |
| 14 | Цветной diff файла | done |
| 15 | Действия над файлом: stage/unstage/restore/blame | done |
| 16 | Лог коммитов + git show по клику | done |
| 17 | Список stash | done |
| 18 | Ahead/behind + local/remote commit counts | done |
| 19 | Hunk-level stage (git apply --cached guided UI) | done |
| 20 | Side-by-side / word-diff toggle | done |
| 21 | Фильтр файлов по имени в статус-панели | done |
| 22 | Сравнение двух коммитов (Ctrl/Shift-клик A/B) | done |
| 23 | Reflog-панель (read-only) | done |
| 24 | Remote tracking chips на ветках (ahead/behind per branch) | done |

## C. Команды, превью, мастер (25–34)

| # | Улучшение | Статус |
|---|-----------|--------|
| 25 | Обязательное превью + destructive confirm | done |
| 26 | Копирование cmdline / diff / output | done |
| 27 | Prefill параметров из контекста (path/branch) | done |
| 28 | Валидация обязательных параметров | done |
| 29 | Мастер «первый коммит» (guided flow) | done |
| 30 | Мастер «синхронизация с remote» | done |
| 31 | Шаблоны сообщений коммита (conventional) | done |
| 32 | Dry-run бейдж на превью для безопасных команд | done |
| 33 | История сессии → повтор параметров (replay) | done |
| 34 | Расширение каталога до ≥60 команд (worktree/bisect/…) | done |

## D. Обучение и LLM (35–42)

| # | Улучшение | Статус |
|---|-----------|--------|
| 35 | Rule-based рекомендации | done |
| 36 | LLM-обогащение рекомендаций | done |
| 37 | Спросить наставника (askAdvice) | done |
| 38 | Настройки LLM URL/model/key | done |
| 39 | Режим «урок»: учебный репо + мастера | done |
| 40 | Карточки «почему эта команда» с ссылкой на explanation | done |
| 41 | Offline fallback тексты, если LLM недоступен (явный UX) | done |
| 42 | Стриминг ответа LLM в UI (SSE token-by-token) | done |

## E. Надёжность, безопасность, качество (43–50)

| # | Улучшение | Статус |
|---|-----------|--------|
| 43 | qmllint 0 warnings | done |
| 44 | Smoke с dump-status + recommendations | done |
| 45 | LLM только localhost/LAN | done |
| 46 | Unit-тесты PreviewEngine / porcelain / hunks | done |
| 47 | CLI: `--self-check` (catalog+git+porcelain) | done |
| 48 | Обработка detached HEAD / bare / submodule root | done |
| 49 | Таймауты git + отмена длинных операций (cancel UI) | done |
| 50 | Журнал аудита сессии (JSONL) | done |

## F. Дополнительно до полного покрытия (≥50)

| # | Улучшение | Статус |
|---|-----------|--------|
| 51 | Контекстные подсказки в ParamForm (placeholder из synopsis) | done |
| 52 | Индикатор «рабочее дерево чистое / dirty» в title bar | done |
| 53 | Drag-and-drop папки на Welcome → open repo | done |
| 54 | Мультивыбор файлов для add_path | done |
| 55 | Сравнение рабочей копии с upstream (rev-list narrative) | done |
| 56 | Панель «что изменилось после execute» (auto status delta) | done |
| 57 | Локализация строк UI через ki18n domain | done |
| 58 | Контраст WCAG для copper/mist на ink | done |
| 59 | RPM/AppImage packaging smoke в CI script | done |
| 60 | Документированный ACCEPTANCE checklist | done |

---

## Очередь

**Sprint 1–2:** закрыты.  
**Sprint 3 (полировка приёмки):** закрыт по пунктам 57–59.

---

## ACCEPTANCE checklist (п. 60)

1. `cmake --build build` · `bin/git-lab-tests` → `UNIT_OK` (porcelain + preview + hunks).
2. `bin/git-lab --self-check` → `SELF_CHECK_OK` (catalog ≥60).
3. `scripts/smoke-test.sh` → `SMOKE_OK`.
4. `qmllint-qt6 -I build qml/*.qml` → 0 warnings.
5. `scripts/ci-packaging-smoke.sh` → `PACKAGING_SMOKE_OK`.
6. UI: breadcrumb, SplitView, Ctrl+K, hunks tab, compare Ctrl/Shift на коммитах, replay в истории, урок (учебный репо), SSE в советах, «отмена» в busy-chip.
7. Этот файл: 60 пунктов со статусом `done` соответствуют работающему коду.
