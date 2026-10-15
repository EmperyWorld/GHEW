# Структура файлов

```
ghscr/
├── README.md
├── LICENSE
├── CMakeLists.txt
├── .gitignore
├── docs/
│   ├── ARCHITECTURE.md      # слои, потоки данных
│   ├── DEVELOPMENT.md       # сборка и соглашения
│   ├── COMMAND_CATALOG.md   # схема JSON команд
│   └── STRUCTURE.md         # этот файл
├── src/
│   ├── main.cpp
│   ├── app/
│   │   ├── AppController.h
│   │   └── AppController.cpp
│   └── core/
│       ├── CommandSpec.{h,cpp}
│       ├── CommandCatalog.{h,cpp}
│       ├── RepoContext.{h,cpp}
│       ├── GitRunner.{h,cpp}
│       └── PreviewEngine.{h,cpp}
├── qml/                     # плоский QML-модуль GitLab (для qt_add_qml_module)
│   ├── Main.qml
│   ├── WelcomePage.qml
│   ├── WorkspacePage.qml
│   ├── RepoHeader.qml
│   ├── CommandList.qml
│   ├── PreviewPanel.qml
│   └── OutputTerminal.qml
├── data/
│   └── commands/            # JSON-каталог (inspect, staging, …)
├── packaging/
│   ├── README.md
│   ├── git-lab.spec
│   ├── git-lab.desktop
│   └── appimage/build.sh
└── resources/
    └── icons/               # иконка приложения (placeholder)
```

Дальше по плану: расширить `data/commands/`, специализированные превью, формы параметров в QML, довести RPM/AppImage.
