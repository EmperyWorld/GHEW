#pragma once

#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QVector>

struct CommandParam {
    QString name;
    QString label;
    QString type; // bool | string | text | path | choice
    QString flag;
    QVariant defaultValue;
    QStringList options;
    bool optional = false;
    bool keepEmpty = false;
};

enum class DangerLevel { Safe, Mutates, Destructive };

struct CommandSpec {
    QString id;
    QString group;
    QString title;
    QString synopsis;
    DangerLevel danger = DangerLevel::Safe;
    QString explanation;
    QString runner = QStringLiteral("git"); // git | gh | file
    QStringList gitBase;
    QVector<CommandParam> params;
    QString previewStrategy;

    static DangerLevel dangerFromString(const QString &value);
    static QString dangerToString(DangerLevel level);
};
