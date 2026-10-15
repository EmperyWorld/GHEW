#pragma once

#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QVector>

struct MentorAskUser {
    QString prompt;
    QStringList options;
    /// Prompt alone is enough — the student may answer in free text, not only chips.
    bool isValid() const { return !prompt.isEmpty(); }
};

struct MentorCommand {
    QString id;
    QVariantMap args;
};

struct MentorReply {
    QString answer;
    MentorAskUser askUser;
    QVector<MentorCommand> commands;
};

/// Strip Gemma/CoT leakage (Self-Correction, English checklists, duplicates)
/// and extract optional <<<ASK_USER>>> tool block.
MentorReply parseMentorReply(const QString &raw);
