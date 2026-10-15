#include "CommandSpec.h"

DangerLevel CommandSpec::dangerFromString(const QString &value)
{
    if (value == QLatin1String("destructive")) {
        return DangerLevel::Destructive;
    }
    if (value == QLatin1String("mutates")) {
        return DangerLevel::Mutates;
    }
    return DangerLevel::Safe;
}

QString CommandSpec::dangerToString(DangerLevel level)
{
    switch (level) {
    case DangerLevel::Destructive:
        return QStringLiteral("destructive");
    case DangerLevel::Mutates:
        return QStringLiteral("mutates");
    case DangerLevel::Safe:
        break;
    }
    return QStringLiteral("safe");
}
