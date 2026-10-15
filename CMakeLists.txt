cmake_minimum_required(VERSION 3.22)
project(ghew VERSION 0.10.4 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(QT_QML_GENERATE_QMLDIR_INI OFF)

include(GNUInstallDirs)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 Network)
find_package(KF6I18n REQUIRED)
find_package(KF6Kirigami REQUIRED)

qt_standard_project_setup(REQUIRES 6.5)
qt_policy(SET QTP0001 NEW)

set(ghew_QML_FILES
  qml/Main.qml
  qml/WelcomePage.qml
  qml/WorkspacePage.qml
  qml/RepoHeader.qml
  qml/CommandList.qml
  qml/ParamForm.qml
  qml/PreviewPanel.qml
  qml/OutputTerminal.qml
  qml/HistoryPanel.qml
  qml/RepoStatusPanel.qml
  qml/AdvicePanel.qml
  qml/DiffPanel.qml
  qml/SettingsPanel.qml
  qml/QuickActionBar.qml
  qml/InspectPanel.qml
  qml/WizardPanel.qml
  qml/HunkPanel.qml
  qml/CommandPalette.qml
  qml/BranchDialog.qml
  qml/TagDialog.qml
  qml/I18n.qml
  qml/Theme.qml
  qml/ScrollPane.qml
  qml/ChipFlow.qml
  qml/SoftBar.qml
  qml/ActionButton.qml
  qml/StatPill.qml
  qml/SectionHead.qml
  qml/SurfaceCard.qml
  qml/EmptyHint.qml
  qml/TabRail.qml
  qml/CommandMast.qml
  qml/NextSteps.qml
  qml/KeyLegend.qml
  qml/OperationBanner.qml
  qml/SidebarColumn.qml
  qml/WorkspaceDetail.qml
  qml/ChipButton.qml
  qml/MeterTrack.qml
  qml/DirtyBreakdown.qml
  qml/TrackingBar.qml
  qml/RepoPulse.qml
  qml/RefInventory.qml
  qml/BranchLane.qml
  qml/RecentCommandStrip.qml
  qml/DangerScale.qml
  qml/CommandFacts.qml
  qml/CommitDock.qml
  qml/DiffChrome.qml
  qml/InspectChrome.qml
  qml/OverviewDeck.qml
  qml/HistoryEntry.qml
  qml/SessionFooter.qml
  qml/MentorDesk.qml
  qml/GithubDesk.qml
  qml/GithubServiceDeck.qml
  qml/GithubAccessDeck.qml
  qml/GithubIdentityDeck.qml
)

foreach(_qml_file IN LISTS ghew_QML_FILES)
  get_filename_component(_qml_name "${_qml_file}" NAME)
  set_source_files_properties("${_qml_file}" PROPERTIES QT_RESOURCE_ALIAS "${_qml_name}")
endforeach()

set_source_files_properties(qml/I18n.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

set(ghew_COMMAND_JSON
  data/commands/inspect.json
  data/commands/staging.json
  data/commands/commit.json
  data/commands/branch.json
  data/commands/remote.json
  data/commands/merge.json
  data/commands/stash.json
  data/commands/tag.json
  data/commands/undo.json
  data/commands/advanced.json
  data/commands/extra.json
  data/commands/github.json
)

qt_add_executable(ghew src/main.cpp)

qt_add_resources(ghew "ghew_commands"
  PREFIX "/ghew"
  BASE data
  FILES ${ghew_COMMAND_JSON}
)

qt_add_resources(ghew "ghew_brand"
  PREFIX "/ghew/brand"
  BASE resources/icons
  FILES
    resources/icons/ghew-logo.png
    resources/icons/ghew.png
)

qt_add_qml_module(ghew
  URI Ghew
  VERSION 1.0
  NO_IMPORT_SCAN
  SOURCES
    src/app/AppController.cpp
    src/app/AppController.h
    src/app/GithubDeskDispatch.cpp
    src/core/RepoContext.cpp
    src/core/RepoContext.h
    src/core/CommandCatalog.cpp
    src/core/CommandCatalog.h
    src/core/GitRunner.cpp
    src/core/GitRunner.h
    src/core/PreviewEngine.cpp
    src/core/PreviewEngine.h
    src/core/CommandSpec.cpp
    src/core/CommandSpec.h
    src/core/HistoryModel.cpp
    src/core/HistoryModel.h
    src/core/StatusFileModel.cpp
    src/core/StatusFileModel.h
    src/core/BranchListModel.cpp
    src/core/BranchListModel.h
    src/core/RecommendationModel.cpp
    src/core/RecommendationModel.h
    src/core/LlmClient.cpp
    src/core/LlmClient.h
    src/core/Advisor.cpp
    src/core/Advisor.h
    src/core/GithubClient.cpp
    src/core/GithubClient.h
    src/core/GithubRemote.cpp
    src/core/GithubRemote.h
    src/core/GithubScaffold.cpp
    src/core/GithubScaffold.h
    src/core/GithubPolicy.cpp
    src/core/GithubPolicy.h
    src/core/GithubLifecycle.cpp
    src/core/GithubLifecycle.h
    src/core/GithubServices.cpp
    src/core/GithubServices.h
    src/core/GithubAccess.cpp
    src/core/GithubAccess.h
    src/core/GithubIdentity.cpp
    src/core/GithubIdentity.h
    src/core/GithubDeskActions.cpp
    src/core/GithubDeskActions.h
    src/core/MentorReply.cpp
    src/core/MentorReply.h
    src/core/RecentReposModel.cpp
    src/core/RecentReposModel.h
    src/core/EphemeralRepoPath.cpp
    src/core/EphemeralRepoPath.h
    src/core/CommitLogModel.cpp
    src/core/CommitLogModel.h
    src/core/StashListModel.cpp
    src/core/StashListModel.h
    src/core/FavoritesModel.cpp
    src/core/FavoritesModel.h
    src/core/GitStatusParse.cpp
    src/core/GitStatusParse.h
    src/core/ReflogListModel.cpp
    src/core/ReflogListModel.h
    src/core/HunkParse.cpp
    src/core/HunkParse.h
    src/core/HunkListModel.cpp
    src/core/HunkListModel.h
    src/core/RecentCommandsModel.cpp
    src/core/RecentCommandsModel.h
    src/core/TagListModel.cpp
    src/core/TagListModel.h
    src/core/RemoteListModel.cpp
    src/core/RemoteListModel.h
    src/core/WorktreeListModel.cpp
    src/core/WorktreeListModel.h
    src/core/SubmoduleListModel.cpp
    src/core/SubmoduleListModel.h
    src/core/HookListModel.cpp
    src/core/HookListModel.h
  QML_FILES ${ghew_QML_FILES}
)

target_include_directories(ghew PRIVATE src src/app src/core)

target_link_libraries(ghew PRIVATE
  Qt6::Core
  Qt6::Gui
  Qt6::Qml
  Qt6::Quick
  Qt6::QuickControls2
  Qt6::Network
  KF6::I18n
  KF6::I18nQml
  KF6::Kirigami
)

target_compile_options(ghew PRIVATE
  -Wall -Wextra -Wpedantic
)

target_compile_definitions(ghew PRIVATE
  GHEW_VERSION="${PROJECT_VERSION}"
  GHEW_SOURCE_DATA_DIR="${CMAKE_SOURCE_DIR}/data"
  # Compatibility aliases for transitional code
  GIT_LAB_VERSION="${PROJECT_VERSION}"
  GIT_LAB_SOURCE_DATA_DIR="${CMAKE_SOURCE_DIR}/data"
)

set_target_properties(ghew PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

install(TARGETS ghew RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY data/ DESTINATION ${CMAKE_INSTALL_DATADIR}/ghew)
install(FILES packaging/ghew.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES resources/icons/ghew.png
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps
        RENAME ghew.png)
install(FILES resources/icons/hicolor-128-ghew.png
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/128x128/apps
        RENAME ghew.png)

add_executable(ghew-tests
  tests/test_status_parse.cpp
  tests/test_llm_live.cpp
  src/core/LlmClient.cpp
  src/core/GitStatusParse.cpp
  src/core/HunkParse.cpp
  src/core/PreviewEngine.cpp
  src/core/CommandSpec.cpp
  src/core/GitRunner.cpp
  src/core/CommitLogModel.cpp
  src/core/MentorReply.cpp
  src/core/EphemeralRepoPath.cpp
  src/core/GithubRemote.cpp
  src/core/GithubScaffold.cpp
  src/core/GithubPolicy.cpp
  src/core/GithubLifecycle.cpp
  src/core/GithubServices.cpp
  src/core/GithubAccess.cpp
  src/core/GithubIdentity.cpp
  src/core/GithubDeskActions.cpp
)
set_target_properties(ghew-tests PROPERTIES AUTOMOC ON)
target_include_directories(ghew-tests PRIVATE src src/core)
target_link_libraries(ghew-tests PRIVATE Qt6::Core Qt6::Network Qt6::Qml)
target_compile_definitions(ghew-tests PRIVATE
  GHEW_VERSION="${PROJECT_VERSION}"
  GIT_LAB_VERSION="${PROJECT_VERSION}"
  GIT_LAB_UNIT_TEST=1
)
set_target_properties(ghew-tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
enable_testing()
add_test(NAME status_parse COMMAND ghew-tests)
