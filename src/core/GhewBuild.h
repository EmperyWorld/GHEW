#pragma once

// CMake defines these via -DGHEW_*="…". Fallbacks keep clangd / IDEs happy
// when compile_commands is missing or stale.
#ifndef GHEW_VERSION
#define GHEW_VERSION "0.0.0-dev"
#endif

#ifndef GHEW_SOURCE_DATA_DIR
#define GHEW_SOURCE_DATA_DIR ""
#endif
