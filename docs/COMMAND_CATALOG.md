# Формат каталога команд

Файлы: `data/commands/*.json`. Каждый файл — массив объектов команд или объект с полем `commands`.

## Схема команды

```json
{
  "id": "status",
  "group": "inspect",
  "title": "Статус рабочей копии",
  "synopsis": "git status [options]",
  "danger": "safe",
  "explanation": "Markdown-текст пояснения…",
  "git": ["status"],
  "params": [
    {
      "name": "short",
      "label": "Короткий формат",
      "type": "bool",
      "flag": "-sb",
      "default": true
    }
  ],
  "preview": "status"
}
```

### Поля

| Поле | Тип | Описание |
|------|-----|----------|
| `id` | string | Уникальный идентификатор |
| `group` | string | Группа для sidebar |
| `title` | string | Заголовок в UI |
| `synopsis` | string | Краткий синтаксис |
| `danger` | enum | `safe` \| `mutates` \| `destructive` |
| `explanation` | string | Markdown, расширенное пояснение |
| `git` | string[] | Базовые аргументы после `git` |
| `params` | array | Параметры формы |
| `preview` | string | Имя стратегии превью в `PreviewEngine` |

### Типы параметров

| `type` | UI | Сборка argv |
|--------|----|-------------|
| `bool` | переключатель | если true — добавить `flag` |
| `string` | поле ввода | значение как аргумент или после `flag` |
| `text` | многострочное | например `-m` message |
| `path` | путь/файл относительно репо | один аргумент |
| `choice` | список (`options`) | выбранное значение |

## Группы (v1)

| `group` | Название в UI |
|---------|----------------|
| `inspect` | Просмотр |
| `staging` | Индекс |
| `commit` | Коммиты |
| `branch` | Ветки |
| `remote` | Remote |
| `merge` | Merge / Rebase |
| `stash` | Stash |
| `tag` | Теги |
| `undo` | Отмена |
| `advanced` | Advanced |

## Стратегии превью

Реализуются в C++ (`PreviewEngine`):

| Имя | Поведение |
|-----|-----------|
| `generic` | cmdline + текущий `status -sb` + предупреждение по danger |
| `status` | показать, что будет выведен статус (без изменений) |
| `diff` | краткий diffstat |
| `add` | список файлов, которые попадут в индекс |
| `commit` | staged diffstat + сообщение |
| `branch_switch` | текущая → целевая ветка |
| `reset` | что будет сброшено; для `--hard` — список потерь |
| `push` | ahead/behind + `push --dry-run` |
| `clean` | `clean -n` список |
| `merge` | какие коммиты войдут / конфликтный риск |

Неизвестная стратегия → `generic`.
