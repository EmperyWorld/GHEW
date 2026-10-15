#pragma once

#include <QString>

struct StatusFileEntry {
    QString path;
    QString code;   // porcelain XY
    QString state;  // staged | unstaged | untracked | conflict | mixed
    QString summary;
};
