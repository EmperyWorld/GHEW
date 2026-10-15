#include "GithubPolicy.h"

namespace {

GithubFlag readBool(const QJsonObject &object, const char *key)
{
    GithubFlag flag;
    const QString name = QString::fromLatin1(key);
    const QJsonValue value = object.value(name);
    if (!object.contains(name) || value.isNull() || !value.isBool())
        return flag;
    flag.known = true;
    flag.value = value.toBool();
    return flag;
}

GithubFlag readSecurity(const QJsonObject &repo, const char *feature)
{
    GithubFlag flag;
    const QJsonObject block = repo.value(QStringLiteral("security_and_analysis"))
                                  .toObject()
                                  .value(QString::fromLatin1(feature))
                                  .toObject();
    const QString status = block.value(QStringLiteral("status")).toString();
    if (status != QLatin1String("enabled") && status != QLatin1String("disabled"))
        return flag;
    flag.known = true;
    flag.value = status == QLatin1String("enabled");
    return flag;
}

void readChoice(const QJsonObject &object, const char *key, const QStringList &allowed, bool *known, QString *value)
{
    const QString name = QString::fromLatin1(key);
    const QJsonValue raw = object.value(name);
    if (!object.contains(name) || raw.isNull())
        return;
    const QString text = raw.toString();
    if (!allowed.contains(text))
        return;
    *known = true;
    *value = text;
}

void fillBool(GithubFlag *flag, const QJsonObject &object, const char *key)
{
    if (flag->known)
        return;
    *flag = readBool(object, key);
}

GithubFlag draftFlag(const QVariantMap &draft, const char *key)
{
    GithubFlag flag;
    const QString name = QString::fromLatin1(key);
    flag.known = draft.value(name + QStringLiteral("Known")).toBool();
    flag.value = draft.value(name).toBool();
    return flag;
}

void addBool(QStringList *args, const char *field, const GithubFlag &want, const GithubFlag &have)
{
    if (!want.known || !have.known || want.value == have.value)
        return;
    args->append(QStringLiteral("-F"));
    args->append(QStringLiteral("%1=%2").arg(QLatin1String(field), want.value ? QStringLiteral("true")
                                                                                : QStringLiteral("false")));
}

bool addChoice(QStringList *args, const char *field, bool wantKnown, const QString &want, bool haveKnown,
               const QString &have, const QStringList &allowed, QString *error)
{
    if (!wantKnown || !haveKnown || want == have)
        return true;
    if (!allowed.contains(want)) {
        if (error)
            *error = QStringLiteral("Недопустимое значение «%1».").arg(QString::fromLatin1(field));
        return false;
    }
    args->append(QStringLiteral("-f"));
    args->append(QStringLiteral("%1=%2").arg(QLatin1String(field), want));
    return true;
}

void addSecurity(QStringList *args, const char *feature, const GithubFlag &want, const GithubFlag &have)
{
    if (!want.known || !have.known || want.value == have.value)
        return;
    args->append(QStringLiteral("-f"));
    args->append(QStringLiteral("security_and_analysis[%1][status]=%2")
                     .arg(QLatin1String(feature), want.value ? QStringLiteral("enabled") : QStringLiteral("disabled")));
}

const QStringList kSquashTitle{QStringLiteral("PR_TITLE"), QStringLiteral("COMMIT_OR_PR_TITLE")};
const QStringList kSquashBody{QStringLiteral("PR_BODY"), QStringLiteral("COMMIT_MESSAGES"), QStringLiteral("BLANK")};
const QStringList kMergeTitle{QStringLiteral("PR_TITLE"), QStringLiteral("MERGE_MESSAGE")};
const QStringList kMergeBody{QStringLiteral("PR_BODY"), QStringLiteral("PR_TITLE"), QStringLiteral("BLANK")};
const QStringList kPrPolicy{QStringLiteral("all"), QStringLiteral("collaborators_only")};

} // namespace

GithubPolicy githubPolicyFromApi(const QJsonObject &repo)
{
    GithubPolicy policy;
    policy.loaded = repo.contains(QStringLiteral("full_name")) || repo.contains(QStringLiteral("id"));
    policy.mergeCommit = readBool(repo, "allow_merge_commit");
    policy.squashMerge = readBool(repo, "allow_squash_merge");
    policy.rebaseMerge = readBool(repo, "allow_rebase_merge");
    policy.autoMerge = readBool(repo, "allow_auto_merge");
    policy.updateBranch = readBool(repo, "allow_update_branch");
    policy.allowForking = readBool(repo, "allow_forking");
    policy.webSignoff = readBool(repo, "web_commit_signoff_required");
    policy.pullRequests = readBool(repo, "has_pull_requests");
    policy.archived = readBool(repo, "archived");
    policy.advancedSecurity = readSecurity(repo, "advanced_security");
    policy.secretScanning = readSecurity(repo, "secret_scanning");
    policy.pushProtection = readSecurity(repo, "secret_scanning_push_protection");
    readChoice(repo, "squash_merge_commit_title", kSquashTitle, &policy.squashTitleKnown, &policy.squashTitle);
    readChoice(repo, "squash_merge_commit_message", kSquashBody, &policy.squashBodyKnown, &policy.squashBody);
    readChoice(repo, "merge_commit_title", kMergeTitle, &policy.mergeTitleKnown, &policy.mergeTitle);
    readChoice(repo, "merge_commit_message", kMergeBody, &policy.mergeBodyKnown, &policy.mergeBody);
    readChoice(repo, "pull_request_creation_policy", kPrPolicy, &policy.prPolicyKnown, &policy.prPolicy);
    return policy;
}

void githubPolicyFillFromView(GithubPolicy *policy, const QJsonObject &view)
{
    if (!policy)
        return;
    fillBool(&policy->mergeCommit, view, "mergeCommitAllowed");
    fillBool(&policy->squashMerge, view, "squashMergeAllowed");
    fillBool(&policy->rebaseMerge, view, "rebaseMergeAllowed");
    fillBool(&policy->archived, view, "isArchived");
    if (policy->mergeCommit.known || policy->squashMerge.known || policy->rebaseMerge.known || policy->archived.known)
        policy->loaded = true;
}

QVariantMap githubPolicyToMap(const GithubPolicy &policy)
{
    QVariantMap map;
    const auto put = [&map](const char *key, const GithubFlag &flag) {
        const QString name = QString::fromLatin1(key);
        map.insert(name, flag.value);
        map.insert(name + QStringLiteral("Known"), flag.known);
    };
    put("mergeCommit", policy.mergeCommit);
    put("squashMerge", policy.squashMerge);
    put("rebaseMerge", policy.rebaseMerge);
    put("autoMerge", policy.autoMerge);
    put("updateBranch", policy.updateBranch);
    put("allowForking", policy.allowForking);
    put("webSignoff", policy.webSignoff);
    put("pullRequests", policy.pullRequests);
    put("advancedSecurity", policy.advancedSecurity);
    put("secretScanning", policy.secretScanning);
    put("pushProtection", policy.pushProtection);
    put("archived", policy.archived);
    map.insert(QStringLiteral("squashTitle"), policy.squashTitle);
    map.insert(QStringLiteral("squashTitleKnown"), policy.squashTitleKnown);
    map.insert(QStringLiteral("squashBody"), policy.squashBody);
    map.insert(QStringLiteral("squashBodyKnown"), policy.squashBodyKnown);
    map.insert(QStringLiteral("mergeTitle"), policy.mergeTitle);
    map.insert(QStringLiteral("mergeTitleKnown"), policy.mergeTitleKnown);
    map.insert(QStringLiteral("mergeBody"), policy.mergeBody);
    map.insert(QStringLiteral("mergeBodyKnown"), policy.mergeBodyKnown);
    map.insert(QStringLiteral("prPolicy"), policy.prPolicy);
    map.insert(QStringLiteral("prPolicyKnown"), policy.prPolicyKnown);
    map.insert(QStringLiteral("loaded"), policy.loaded);
    return map;
}

GithubPolicy githubPolicyFromDraft(const QVariantMap &draft)
{
    GithubPolicy policy;
    policy.loaded = true;
    policy.mergeCommit = draftFlag(draft, "mergeCommit");
    policy.squashMerge = draftFlag(draft, "squashMerge");
    policy.rebaseMerge = draftFlag(draft, "rebaseMerge");
    policy.autoMerge = draftFlag(draft, "autoMerge");
    policy.updateBranch = draftFlag(draft, "updateBranch");
    policy.allowForking = draftFlag(draft, "allowForking");
    policy.webSignoff = draftFlag(draft, "webSignoff");
    policy.pullRequests = draftFlag(draft, "pullRequests");
    policy.advancedSecurity = draftFlag(draft, "advancedSecurity");
    policy.secretScanning = draftFlag(draft, "secretScanning");
    policy.pushProtection = draftFlag(draft, "pushProtection");
    policy.squashTitleKnown = draft.value(QStringLiteral("squashTitleKnown")).toBool();
    policy.squashTitle = draft.value(QStringLiteral("squashTitle")).toString();
    policy.squashBodyKnown = draft.value(QStringLiteral("squashBodyKnown")).toBool();
    policy.squashBody = draft.value(QStringLiteral("squashBody")).toString();
    policy.mergeTitleKnown = draft.value(QStringLiteral("mergeTitleKnown")).toBool();
    policy.mergeTitle = draft.value(QStringLiteral("mergeTitle")).toString();
    policy.mergeBodyKnown = draft.value(QStringLiteral("mergeBodyKnown")).toBool();
    policy.mergeBody = draft.value(QStringLiteral("mergeBody")).toString();
    policy.prPolicyKnown = draft.value(QStringLiteral("prPolicyKnown")).toBool();
    policy.prPolicy = draft.value(QStringLiteral("prPolicy")).toString();
    return policy;
}

QStringList githubPolicyPatchArgs(const GithubPolicy &want, const GithubPolicy &have, QString *error)
{
    if (error)
        error->clear();

    const bool scanKnown = want.secretScanning.known && have.secretScanning.known;
    const bool pushKnown = want.pushProtection.known && have.pushProtection.known;
    const bool scanOn = scanKnown ? want.secretScanning.value
                                  : (have.secretScanning.known && have.secretScanning.value);
    const bool pushOn = pushKnown ? want.pushProtection.value
                                  : (have.pushProtection.known && have.pushProtection.value);
    if ((scanKnown || pushKnown) && pushOn && !scanOn) {
        if (error)
            *error = QStringLiteral("Блокировка пуша секретов требует включённый поиск секретов.");
        return {};
    }

    QStringList args;
    addBool(&args, "allow_merge_commit", want.mergeCommit, have.mergeCommit);
    addBool(&args, "allow_squash_merge", want.squashMerge, have.squashMerge);
    addBool(&args, "allow_rebase_merge", want.rebaseMerge, have.rebaseMerge);
    addBool(&args, "allow_auto_merge", want.autoMerge, have.autoMerge);
    addBool(&args, "allow_update_branch", want.updateBranch, have.updateBranch);
    addBool(&args, "allow_forking", want.allowForking, have.allowForking);
    addBool(&args, "web_commit_signoff_required", want.webSignoff, have.webSignoff);
    addBool(&args, "has_pull_requests", want.pullRequests, have.pullRequests);
    if (!addChoice(&args, "squash_merge_commit_title", want.squashTitleKnown, want.squashTitle, have.squashTitleKnown,
                   have.squashTitle, kSquashTitle, error))
        return {};
    if (!addChoice(&args, "squash_merge_commit_message", want.squashBodyKnown, want.squashBody, have.squashBodyKnown,
                   have.squashBody, kSquashBody, error))
        return {};
    if (!addChoice(&args, "merge_commit_title", want.mergeTitleKnown, want.mergeTitle, have.mergeTitleKnown,
                   have.mergeTitle, kMergeTitle, error))
        return {};
    if (!addChoice(&args, "merge_commit_message", want.mergeBodyKnown, want.mergeBody, have.mergeBodyKnown,
                   have.mergeBody, kMergeBody, error))
        return {};
    if (!addChoice(&args, "pull_request_creation_policy", want.prPolicyKnown, want.prPolicy, have.prPolicyKnown,
                   have.prPolicy, kPrPolicy, error))
        return {};
    addSecurity(&args, "advanced_security", want.advancedSecurity, have.advancedSecurity);
    addSecurity(&args, "secret_scanning", want.secretScanning, have.secretScanning);
    addSecurity(&args, "secret_scanning_push_protection", want.pushProtection, have.pushProtection);
    return args;
}
