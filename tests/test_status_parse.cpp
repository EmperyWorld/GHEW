#include "core/GitStatusParse.h"
#include "core/HunkParse.h"
#include "core/PreviewEngine.h"
#include "core/CommandSpec.h"
#include "core/GitRunner.h"
#include "core/CommitLogModel.h"
#include "core/MentorReply.h"
#include "core/EphemeralRepoPath.h"
#include "core/GithubRemote.h"
#include "core/GithubScaffold.h"
#include "core/GithubPolicy.h"
#include "core/GithubLifecycle.h"
#include "core/GithubServices.h"
#include "core/GithubAccess.h"
#include "core/GithubIdentity.h"
#include "core/GithubDeskActions.h"

#include <QJsonDocument>
#include <QJsonObject>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>

#include <cstdio>

#define CHECK(cond)                                                                                                    \
    do {                                                                                                               \
        if (!(cond)) {                                                                                                 \
            std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);                                       \
            return 1;                                                                                                  \
        }                                                                                                              \
    } while (0)

static int testPorcelain()
{
    const auto r = parsePorcelainStatus(QStringLiteral(
        "M  staged.txt\n"
        " M unstaged.txt\n"
        "?? new.txt\n"
        "MM both.txt\n"
        "R  old.txt -> renamed.txt\n"
        "UU conflict.txt\n"));

    CHECK(r.files.size() == 6);
    CHECK(r.stagedCount >= 2);
    CHECK(r.unstagedCount >= 2);
    CHECK(r.untrackedCount == 1);
    CHECK(r.conflictCount == 1);

    bool sawRename = false;
    bool sawConflict = false;
    bool sawUntracked = false;
    for (const StatusFileEntry &e : r.files) {
        if (e.path == QLatin1String("renamed.txt") && e.code.startsWith(QLatin1Char('R')))
            sawRename = true;
        if (e.state == QLatin1String("conflict"))
            sawConflict = true;
        if (e.path == QLatin1String("new.txt") && e.state == QLatin1String("untracked"))
            sawUntracked = true;
    }
    CHECK(sawRename);
    CHECK(sawConflict);
    CHECK(sawUntracked);

    CHECK(classifyPorcelainState(QLatin1Char('M'), QLatin1Char(' ')) == QLatin1String("staged"));
    CHECK(classifyPorcelainState(QLatin1Char(' '), QLatin1Char('M')) == QLatin1String("unstaged"));
    CHECK(classifyPorcelainState(QLatin1Char('?'), QLatin1Char('?')) == QLatin1String("untracked"));
    return 0;
}

static int testPreviewSafe()
{
    GitRunner runner(3000);
    PreviewEngine engine(&runner);
    CommandSpec spec;
    spec.id = QStringLiteral("status");
    spec.title = QStringLiteral("Status");
    spec.synopsis = QStringLiteral("git status");
    spec.danger = DangerLevel::Safe;
    spec.gitBase = {QStringLiteral("status"), QStringLiteral("-sb")};
    spec.previewStrategy = QStringLiteral("status");

    const PreviewResult pr = engine.build(spec, {}, QDir::tempPath());
    CHECK(pr.ok);
    CHECK(pr.cmdline.contains(QStringLiteral("git status")));
    CHECK(pr.narrative.contains(QStringLiteral("безопасн")));
    return 0;
}

static int testHunks()
{
    const QString diff = QStringLiteral(
        "diff --git a/a.txt b/a.txt\n"
        "--- a/a.txt\n"
        "+++ b/a.txt\n"
        "@@ -1,2 +1,3 @@\n"
        " line1\n"
        "-old\n"
        "+new\n"
        "+extra\n"
        "diff --git a/b.txt b/b.txt\n"
        "--- a/b.txt\n"
        "+++ b/b.txt\n"
        "@@ -1 +1,2 @@\n"
        " x\n"
        "+y\n");
    ParsedDiff p = parseUnifiedDiff(diff);
    CHECK(p.hunks.size() == 2);
    CHECK(p.hunks.at(0).filePath == QLatin1String("a.txt"));
    CHECK(p.hunks.at(1).filePath == QLatin1String("b.txt"));
    QVector<DiffHunk> hunks = p.hunks;
    hunks[1].selected = false;
    const QString patch = buildPatchFromHunks(hunks, true);
    CHECK(patch.contains(QStringLiteral("a.txt")));
    CHECK(!patch.contains(QStringLiteral("b.txt")));
    return 0;
}

static int testCommitFilter()
{
    CommitLogModel model;
    QVector<CommitEntry> list;
    list.push_back({QStringLiteral("aaa111"), QStringLiteral("aaa1111"), QStringLiteral("fix login"),
                    QStringLiteral("Ada"), QStringLiteral("2 days ago")});
    list.push_back({QStringLiteral("bbb222"), QStringLiteral("bbb2222"), QStringLiteral("docs: readme"),
                    QStringLiteral("Bob"), QStringLiteral("1 day ago")});
    list.push_back({QStringLiteral("ccc333"), QStringLiteral("ccc3333"), QStringLiteral("feat: tags"),
                    QStringLiteral("Ada"), QStringLiteral("3 hours ago")});
    model.setCommits(std::move(list));
    CHECK(model.totalCount() == 3);
    CHECK(model.rowCount() == 3);
    model.setFilter(QStringLiteral("ada"));
    CHECK(model.rowCount() == 2);
    model.setFilter(QStringLiteral("docs"));
    CHECK(model.rowCount() == 1);
    CHECK(model.data(model.index(0), CommitLogModel::ShortHashRole).toString() == QLatin1String("bbb2222"));
    model.setFilter(QString());
    CHECK(model.rowCount() == 3);
    return 0;
}

static int testMentorReply()
{
    const QString raw = QStringLiteral(
        "• Max 12 sentences? Yes.\n"
        "• Russian? Yes.\n"
        "Self-Correction: draft in English first.\n"
        "diff - ok.\n"
        "Final Polish: Вижу 26 изменённых и 14 новых файлов.\n"
        "\n"
        "1. Посмотреть изменения (`diff`)\n"
        "2. Добавить выбранные (`add_path`)\n"
        "3. Добавить всё (`add_all`)\n"
        "\n"
        "Что планируешь делать?\n"
        "\n"
        "Вижу 26 изменённых и 14 новых файлов.\n"
        "\n"
        "1. Посмотреть изменения (`diff`)\n"
        "2. Добавить выбранные (`add_path`)\n"
        "3. Добавить всё (`add_all`)\n"
        "\n"
        "Что планируешь делать?\n");

    const MentorReply r = parseMentorReply(raw);
    CHECK(r.answer.contains(QStringLiteral("26 изменённых")));
    CHECK(!r.answer.contains(QStringLiteral("Self-Correction")));
    CHECK(!r.answer.contains(QStringLiteral("Max 12")));
    CHECK(!r.answer.contains(QStringLiteral("diff - ok")));
    // Deduped — question moved to ask_user
    CHECK(!r.answer.contains(QStringLiteral("Что планируешь делать?")));
    CHECK(r.askUser.isValid());
    CHECK(r.askUser.prompt.contains(QStringLiteral("планируешь")));
    CHECK(r.askUser.options.size() >= 2);

    const QString withTool = QStringLiteral(
        "Сначала посмотри diff.\n"
        "<<<ASK_USER>>>\n"
        "{\"prompt\":\"Что дальше?\",\"options\":[\"diff\",\"add_all\"]}\n"
        "<<<END_ASK_USER>>>\n");
    const MentorReply t = parseMentorReply(withTool);
    CHECK(t.answer == QStringLiteral("Сначала посмотри diff."));
    CHECK(t.askUser.prompt == QStringLiteral("Что дальше?"));
    CHECK(t.askUser.options.size() == 2);
    CHECK(t.askUser.options.at(0) == QStringLiteral("diff"));

    const QString withRun = QStringLiteral(
        "Зафиксируйте индекс.\n"
        "<<<RUN>>>\n"
        "{\"commands\":[{\"id\":\"commit\",\"args\":{\"message\":\"fix ci toolchain\"}}]}\n"
        "<<<END_RUN>>>\n");
    const MentorReply run = parseMentorReply(withRun);
    CHECK(run.answer == QStringLiteral("Зафиксируйте индекс."));
    CHECK(!run.answer.contains(QStringLiteral("<<<RUN>>>")));
    CHECK(run.commands.size() == 1);
    CHECK(run.commands.at(0).id == QStringLiteral("commit"));
    CHECK(run.commands.at(0).args.value(QStringLiteral("message")).toString()
          == QStringLiteral("fix ci toolchain"));

    const QString freeAsk = QStringLiteral(
        "Опишите политику безопасности.\n"
        "<<<ASK_USER>>>\n"
        "{\"prompt\":\"Какой текст положить в SECURITY.md?\"}\n"
        "<<<END_ASK_USER>>>\n");
    const MentorReply free = parseMentorReply(freeAsk);
    CHECK(free.askUser.isValid());
    CHECK(free.askUser.prompt.contains(QStringLiteral("SECURITY")));
    CHECK(free.askUser.options.isEmpty());

    const QString dressRun = QStringLiteral(
        "Записываю SECURITY.md.\n"
        "<<<RUN>>>\n"
        "{\"commands\":[{\"id\":\"gh_file\",\"args\":{\"kind\":\"security\","
        "\"body\":\"# Безопасность\\n\\nПишите на security@example.com\\n\"}}]}\n"
        "<<<END_RUN>>>\n");
    const MentorReply dress = parseMentorReply(dressRun);
    CHECK(dress.commands.size() == 1);
    CHECK(dress.commands.at(0).id == QStringLiteral("gh_file"));
    CHECK(dress.commands.at(0).args.value(QStringLiteral("kind")).toString() == QStringLiteral("security"));
    CHECK(dress.commands.at(0).args.value(QStringLiteral("body")).toString().contains(QStringLiteral("security@")));

    CHECK(r.commands.size() >= 2);
    CHECK(r.commands.at(0).id == QStringLiteral("diff"));
    return 0;
}

static int testEphemeralRecent()
{
    CHECK(isEphemeralRepoPath(QStringLiteral("/tmp/git-lab-conflict-20261007141209")));
    CHECK(isEphemeralRepoPath(QStringLiteral("/tmp/ghew-conflict-20261007141209")));
    CHECK(isEphemeralRepoPath(QStringLiteral("/tmp/ghew-init-20261007141209")));
    CHECK(isEphemeralRepoPath(QStringLiteral("/home/owned/.cache/GitLab/git-lab/git-lab-lessons/lesson-20261007-141209")));
    CHECK(!isEphemeralRepoPath(QStringLiteral("/data/repos/eliteew-public-GH-one")));
    CHECK(!isEphemeralRepoPath(QStringLiteral("/home/owned/projects/my-app")));
    return 0;
}

static int testGithubRemote()
{
    const GithubRemote ssh = parseGithubRemote(QStringLiteral("git@github.com:EmperyWorld/ghew.git"));
    CHECK(ssh.ok);
    CHECK(ssh.owner == QLatin1String("EmperyWorld"));
    CHECK(ssh.name == QLatin1String("ghew"));

    const GithubRemote https = parseGithubRemote(QStringLiteral("https://github.com/EmperyWorld/ghew"));
    CHECK(https.ok);
    CHECK(https.host == QLatin1String("github.com"));
    CHECK(https.name == QLatin1String("ghew"));

    const GithubRemote sshUrl = parseGithubRemote(QStringLiteral("ssh://git@github.com/EmperyWorld/ghew.git"));
    CHECK(sshUrl.ok);
    CHECK(sshUrl.owner == QLatin1String("EmperyWorld"));

    const GithubRemote other = parseGithubRemote(QStringLiteral("https://gitlab.com/group/proj.git"));
    CHECK(!other.ok);
    const GithubRemote empty = parseGithubRemote(QStringLiteral(""));
    CHECK(!empty.ok);

    CommandSpec spec;
    spec.runner = QStringLiteral("gh");
    spec.danger = DangerLevel::Mutates;
    spec.gitBase = {QStringLiteral("repo"), QStringLiteral("view")};
    GitRunner runner(3000);
    PreviewEngine engine(&runner);
    const PreviewResult preview = engine.build(spec, {}, QDir::tempPath());
    CHECK(preview.ok);
    CHECK(preview.program == QLatin1String("gh"));
    CHECK(preview.cmdline.startsWith(QStringLiteral("gh repo view")));

    QTemporaryDir dir;
    CHECK(dir.isValid());
    QString error;
    CHECK(writeScaffoldReadme(dir.path(), QStringLiteral("Demo"), QStringLiteral("Зачем"), &error));
    CHECK(QFile::exists(dir.path() + QStringLiteral("/README.md")));
    QFile readme(dir.path() + QStringLiteral("/README.md"));
    CHECK(readme.open(QIODevice::ReadOnly));
    const QString body = QString::fromUtf8(readme.readAll());
    CHECK(body.contains(QStringLiteral("# Demo")));
    CHECK(body.contains(QStringLiteral("Зачем")));
    CHECK(!writeScaffoldReadme(dir.path(), QStringLiteral("Demo"), QStringLiteral("ещё"), &error));

    CHECK(writeScaffoldLicense(dir.path(), QStringLiteral("mit"), QStringLiteral("Owner"), 2026, &error));
    QFile license(dir.path() + QStringLiteral("/LICENSE"));
    CHECK(license.open(QIODevice::ReadOnly));
    const QString licenseBody = QString::fromUtf8(license.readAll());
    CHECK(licenseBody.contains(QStringLiteral("MIT License")));
    CHECK(licenseBody.contains(QStringLiteral("Owner")));
    CHECK(!writeScaffoldLicense(dir.path(), QStringLiteral("bsd2"), QStringLiteral("Owner"), 2026, &error));
    CHECK(!writeScaffoldLicense(dir.path(), QStringLiteral("nope"), QStringLiteral("Owner"), 2026, &error));

    QTemporaryDir desk;
    CHECK(desk.isValid());
    CHECK(dressFileName(desk.path(), QStringLiteral("../README.md")).isEmpty());
    CHECK(dressFileName(desk.path(), QStringLiteral("readme")) == QStringLiteral("README.md"));
    CHECK(writeDressFile(desk.path(), QStringLiteral("gitignore"), QStringLiteral("build/\n"), &error));
    CHECK(readDressFile(desk.path(), QStringLiteral("gitignore")).contains(QStringLiteral("build/")));
    CHECK(writeDressFile(desk.path(), QStringLiteral("gitignore"), QStringLiteral("*.o\n"), &error));
    CHECK(readDressFile(desk.path(), QStringLiteral("gitignore")).contains(QStringLiteral("*.o")));
    CHECK(!readDressFile(desk.path(), QStringLiteral("gitignore")).contains(QStringLiteral("build/")));
    CHECK(!writeDressFile(desk.path(), QStringLiteral("passwd"), QStringLiteral("x"), &error));
    CHECK(!writeDressFile(desk.path(), QStringLiteral("security"), QString(), &error));
    CHECK(error.contains(QStringLiteral("Пустой")));
    CHECK(writeDressFile(desk.path(), QStringLiteral("security"), QStringLiteral("# Безопасность\n"), &error));
    CHECK(dressFilePresent(desk.path(), QStringLiteral("security")));
    CHECK(dressKinds().contains(QStringLiteral("funding")));
    CHECK(scaffoldLicenseText(QStringLiteral("mit"), QStringLiteral("Owner"), 2026).contains(QStringLiteral("MIT License")));
    CHECK(scaffoldLicenseText(QStringLiteral("nope"), QStringLiteral("Owner"), 2026).isEmpty());

    CommandSpec describe;
    describe.runner = QStringLiteral("gh");
    describe.danger = DangerLevel::Mutates;
    describe.gitBase = {QStringLiteral("repo"), QStringLiteral("edit")};
    CommandParam description;
    description.name = QStringLiteral("description");
    description.flag = QStringLiteral("--description");
    description.keepEmpty = true;
    description.type = QStringLiteral("text");
    describe.params.append(description);
    QVariantMap cleared;
    cleared.insert(QStringLiteral("description"), QString());
    const PreviewResult clearPreview = engine.build(describe, cleared, QDir::tempPath());
    CHECK(clearPreview.ok);
    CHECK(clearPreview.argv.contains(QStringLiteral("--description")));

    const QJsonObject repo = QJsonDocument::fromJson(QByteArrayLiteral(
        R"({"id":1,"full_name":"o/r","allow_merge_commit":true,"allow_squash_merge":false,
            "allow_rebase_merge":true,"allow_auto_merge":false,"allow_update_branch":true,
            "allow_forking":false,"web_commit_signoff_required":true,"has_pull_requests":true,
            "archived":false,"pull_request_creation_policy":"all",
            "squash_merge_commit_title":"COMMIT_OR_PR_TITLE","squash_merge_commit_message":"COMMIT_MESSAGES",
            "merge_commit_title":"MERGE_MESSAGE","merge_commit_message":"PR_BODY",
            "security_and_analysis":{"advanced_security":{"status":"disabled"},
              "secret_scanning":{"status":"enabled"},
              "secret_scanning_push_protection":{"status":"disabled"}}})")).object();
    const GithubPolicy have = githubPolicyFromApi(repo);
    CHECK(have.loaded);
    CHECK(have.mergeCommit.known && have.mergeCommit.value);
    CHECK(have.squashMerge.known && !have.squashMerge.value);
    CHECK(have.secretScanning.known && have.secretScanning.value);
    CHECK(have.pushProtection.known && !have.pushProtection.value);
    CHECK(!have.archived.value);

    GithubPolicy want = have;
    want.squashMerge.value = true;
    want.squashBody = QStringLiteral("BLANK");
    error.clear();
    const QStringList turnOn = githubPolicyPatchArgs(want, have, &error);
    CHECK(error.isEmpty());
    CHECK(turnOn.contains(QStringLiteral("allow_squash_merge=true")));
    CHECK(turnOn.contains(QStringLiteral("squash_merge_commit_message=BLANK")));
    CHECK(!turnOn.contains(QStringLiteral("allow_merge_commit=true")));

    want = have;
    want.pushProtection.value = true;
    const QStringList protect = githubPolicyPatchArgs(want, have, &error);
    CHECK(error.isEmpty());
    CHECK(protect.contains(QStringLiteral("security_and_analysis[secret_scanning_push_protection][status]=enabled")));

    want.secretScanning.value = false;
    CHECK(githubPolicyPatchArgs(want, have, &error).isEmpty());
    CHECK(error.contains(QStringLiteral("поиск секретов")));

    want = have;
    want.squashTitle = QStringLiteral("nope");
    CHECK(githubPolicyPatchArgs(want, have, &error).isEmpty());
    CHECK(error.contains(QStringLiteral("squash_merge_commit_title")));

    QJsonObject sparse;
    sparse.insert(QStringLiteral("full_name"), QStringLiteral("o/r"));
    sparse.insert(QStringLiteral("allow_merge_commit"), QJsonValue::Null);
    GithubPolicy missing = githubPolicyFromApi(sparse);
    CHECK(missing.loaded);
    CHECK(!missing.mergeCommit.known);
    QJsonObject view;
    view.insert(QStringLiteral("mergeCommitAllowed"), true);
    view.insert(QStringLiteral("isArchived"), false);
    githubPolicyFillFromView(&missing, view);
    CHECK(missing.mergeCommit.known && missing.mergeCommit.value);
    CHECK(missing.archived.known && !missing.archived.value);
    GithubPolicy haveView = missing;
    missing.mergeCommit.value = false;
    const QStringList fromView = githubPolicyPatchArgs(missing, haveView, &error);
    CHECK(error.isEmpty());
    CHECK(fromView.contains(QStringLiteral("allow_merge_commit=false")));
    CHECK(!fromView.contains(QStringLiteral("archived=false")));
    return 0;
}

static int testGithubLifecycle()
{
    QString error;
    GithubForkRequest safe;
    safe.slug = QStringLiteral("EmperyWorld/ghew");
    safe.forkName = QStringLiteral("ghew-fork");
    safe.defaultBranchOnly = true;
    const QStringList fork = githubForkArgv(safe, &error);
    CHECK(error.isEmpty());
    CHECK(fork.contains(QStringLiteral("EmperyWorld/ghew")));
    CHECK(fork.contains(QStringLiteral("--clone=false")));
    CHECK(fork.contains(QStringLiteral("--default-branch-only")));
    CHECK(!fork.contains(QStringLiteral("--remote")));

    GithubForkRequest rewire = safe;
    rewire.rewireOrigin = true;
    rewire.org = QStringLiteral("EmperyWorld");
    const QStringList wired = githubForkArgv(rewire, &error);
    CHECK(error.isEmpty());
    CHECK(wired.contains(QStringLiteral("--remote")));
    CHECK(!wired.contains(QStringLiteral("EmperyWorld/ghew")));
    CHECK(wired.contains(QStringLiteral("EmperyWorld")));

    safe.forkName = QStringLiteral("bad name");
    CHECK(githubForkArgv(safe, &error).isEmpty());
    CHECK(error.contains(QStringLiteral("Имя форка")));

    const QStringList gone = githubDeleteArgv(QStringLiteral("EmperyWorld/ghew"), QStringLiteral("ghew"),
                                              QStringLiteral("ghew"), &error);
    CHECK(error.isEmpty());
    CHECK(gone.contains(QStringLiteral("EmperyWorld/ghew")));
    CHECK(gone.contains(QStringLiteral("--yes")));
    CHECK(githubDeleteArgv(QStringLiteral("EmperyWorld/ghew"), QStringLiteral("ghew"), QStringLiteral("GHEW"), &error)
              .isEmpty());

    const QStringList move = githubTransferArgv(QStringLiteral("EmperyWorld"), QStringLiteral("ghew"), QStringLiteral("ghew"),
                                                QStringLiteral("poet"), QStringLiteral("ghew-next"), QStringLiteral("12, 34"),
                                                &error);
    CHECK(error.isEmpty());
    CHECK(move.contains(QStringLiteral("repos/{owner}/{repo}/transfer")));
    CHECK(move.contains(QStringLiteral("new_owner=poet")));
    CHECK(move.contains(QStringLiteral("new_name=ghew-next")));
    CHECK(move.contains(QStringLiteral("team_ids[]=12")));
    CHECK(move.contains(QStringLiteral("team_ids[]=34")));
    CHECK(githubTransferArgv(QStringLiteral("EmperyWorld"), QStringLiteral("ghew"), QStringLiteral("ghew"),
                             QStringLiteral("emperyworld"), QString(), QString(), &error)
              .isEmpty());
    CHECK(githubTransferArgv(QStringLiteral("EmperyWorld"), QStringLiteral("ghew"), QStringLiteral("ghew"),
                             QStringLiteral("poet"), QString(), QStringLiteral("0"), &error)
              .isEmpty());

    CHECK(githubLfsArgv(true).contains(QStringLiteral("PUT")));
    CHECK(githubLfsArgv(false).contains(QStringLiteral("DELETE")));
    CHECK(githubLfsArgv(true).contains(QStringLiteral("repos/{owner}/{repo}/lfs")));

    const QStringList python = githubTemplateArgv(QStringLiteral("gitignore"), QStringLiteral("Python"), &error);
    CHECK(error.isEmpty());
    CHECK(python.contains(QStringLiteral("Python")));
    CHECK(githubTemplateArgv(QStringLiteral("license"), QStringLiteral("-mit"), &error).isEmpty());

    QJsonObject pagesBody;
    pagesBody.insert(QStringLiteral("html_url"), QStringLiteral("https://example.github.io/demo/"));
    pagesBody.insert(QStringLiteral("cname"), QStringLiteral(""));
    pagesBody.insert(QStringLiteral("https_enforced"), false);
    QJsonObject source;
    source.insert(QStringLiteral("branch"), QStringLiteral("main"));
    source.insert(QStringLiteral("path"), QStringLiteral("/"));
    pagesBody.insert(QStringLiteral("source"), source);
    const GithubPagesState havePages = githubPagesFromApi(200, pagesBody);
    CHECK(havePages.enabled && havePages.httpsKnown && !havePages.https);
    GithubPagesState wantPages = havePages;
    wantPages.path = QStringLiteral("/docs");
    const QStringList pages = githubPagesArgv(wantPages, havePages, &error);
    CHECK(error.isEmpty());
    CHECK(pages.contains(QStringLiteral("source[path]=/docs")));
    wantPages = githubPagesFromApi(404, {});
    wantPages.known = true;
    wantPages.enabled = true;
    wantPages.branch = QStringLiteral("main");
    wantPages.path = QStringLiteral("/");
    const QStringList createPages = githubPagesArgv(wantPages, githubPagesFromApi(404, {}), &error);
    CHECK(error.isEmpty());
    CHECK(createPages.contains(QStringLiteral("POST")));

    CHECK(githubWikiRemoteUrl(QStringLiteral("git@github.com:EmperyWorld/ghew.git"))
          == QStringLiteral("git@github.com:EmperyWorld/ghew.wiki.git"));
    CHECK(githubWikiRemoteUrl(QStringLiteral("https://github.com/EmperyWorld/ghew"))
          == QStringLiteral("https://github.com/EmperyWorld/ghew.wiki.git"));
    CHECK(!validWikiPageName(QStringLiteral("../Home")));

    QTemporaryDir author;
    QTemporaryDir bare;
    QTemporaryDir cache;
    CHECK(author.isValid() && bare.isValid() && cache.isValid());
    GitRunner git(20000);
    const QString bareRepo = bare.path() + QStringLiteral("/wiki.git");
    CHECK(QDir().mkpath(bareRepo));
    CHECK(git.run(bareRepo, {QStringLiteral("init"), QStringLiteral("--bare")}).ok);
    CHECK(git.run(author.path(), {QStringLiteral("init"), QStringLiteral("-b"), QStringLiteral("main")}).ok);
    CHECK(git.run(author.path(), {QStringLiteral("config"), QStringLiteral("user.name"), QStringLiteral("Owner")}).ok);
    CHECK(git.run(author.path(), {QStringLiteral("config"), QStringLiteral("user.email"), QStringLiteral("poet@eliteew.ru")}).ok);
    const QString wikiCache = cache.path() + QStringLiteral("/wiki");
    const GitRunResult published = publishWikiPage(author.path(), bareRepo, wikiCache, QStringLiteral("Home"),
                                                   QStringLiteral("Привет\n"));
    CHECK(published.ok);
    QTemporaryDir seen;
    CHECK(seen.isValid());
    CHECK(git.run(seen.path(), {QStringLiteral("clone"), bareRepo, QStringLiteral("out")}).ok);
    QFile wikiPage(seen.path() + QStringLiteral("/out/Home.md"));
    CHECK(wikiPage.open(QIODevice::ReadOnly));
    CHECK(QString::fromUtf8(wikiPage.readAll()).contains(QStringLiteral("Привет")));
    CHECK(dressFileName(author.path(), QStringLiteral("codeowners")) == QStringLiteral(".github/CODEOWNERS"));
    CHECK(writeDressFile(author.path(), QStringLiteral("codeowners"), QStringLiteral("* @owner\n"), &error));
    CHECK(readDressFile(author.path(), QStringLiteral("codeowners")).contains(QStringLiteral("@owner")));
    CHECK(dressTemplateText(QStringLiteral("security"), QStringLiteral("GHEW")).contains(QStringLiteral("GHEW")));
    return 0;
}

static int testGithubServices()
{
    QString error;
    const QStringList created = githubIssueWriteArgv({}, QStringLiteral("Нашёл ошибку"), QStringLiteral("Текст\n"), &error);
    CHECK(error.isEmpty());
    CHECK(created.contains(QStringLiteral("create")));
    CHECK(created.contains(QStringLiteral("--title")));
    CHECK(!created.contains(QStringLiteral("--web")));
    const QStringList edited = githubIssueWriteArgv(QStringLiteral("12"), QStringLiteral("Нашёл ошибку"), QStringLiteral("Текст"), &error);
    CHECK(edited.contains(QStringLiteral("12")));
    CHECK(githubIssueWriteArgv(QStringLiteral("12"), QStringLiteral("-секрет"), QStringLiteral("x"), &error).isEmpty());
    CHECK(githubIssueWriteArgv(QStringLiteral("01"), QStringLiteral("Тема"), QStringLiteral("x"), &error).isEmpty());
    const QStringList gone = githubIssueActionArgv(QStringLiteral("4"), QStringLiteral("delete"), {}, &error);
    CHECK(error.isEmpty());
    CHECK(gone.contains(QStringLiteral("--yes")));
    const QStringList locked = githubIssueActionArgv(QStringLiteral("4"), QStringLiteral("lock"), QStringLiteral("resolved"), &error);
    CHECK(locked.contains(QStringLiteral("--reason")));
    CHECK(githubIssueActionArgv(QStringLiteral("4"), QStringLiteral("lock"), QStringLiteral("other"), &error).isEmpty());
    const QStringList assigned = githubIssueLinkArgv(QStringLiteral("4"), QStringLiteral("assignee"), QStringLiteral("@me"), false, &error);
    CHECK(assigned.contains(QStringLiteral("--add-assignee")));
    const QStringList child = githubIssueLinkArgv(QStringLiteral("4"), QStringLiteral("subissue"), QStringLiteral("9"), false, &error);
    CHECK(child.contains(QStringLiteral("--add-sub-issue")));
    const QStringList orphan = githubIssueLinkArgv(QStringLiteral("4"), QStringLiteral("parent"), {}, true, &error);
    CHECK(orphan.contains(QStringLiteral("--remove-parent")));
    CHECK(orphan.size() == 4);
    CHECK(githubIssueLinkArgv(QStringLiteral("4"), QStringLiteral("assignee"), QStringLiteral("-me"), false, &error).isEmpty());

    const QStringList label = githubLabelWriteArgv(QStringLiteral("bug"), QStringLiteral("#E99695"), QStringLiteral("Сбой"), {}, false, &error);
    CHECK(error.isEmpty());
    CHECK(label.contains(QStringLiteral("E99695")));
    CHECK(githubLabelWriteArgv(QStringLiteral("bug"), QStringLiteral("red"), {}, {}, false, &error).isEmpty());
    CHECK(githubLabelDeleteArgv(QStringLiteral("bug"), &error).contains(QStringLiteral("--yes")));

    const QStringList milestone = githubMilestoneArgv({}, QStringLiteral("Версия 1"), QStringLiteral("open"), &error);
    CHECK(milestone.contains(QStringLiteral("POST")));
    CHECK(milestone.contains(QStringLiteral("title=Версия 1")));
    const QStringList dropMilestone = githubMilestoneArgv(QStringLiteral("3"), {}, QStringLiteral("delete"), &error);
    CHECK(dropMilestone.contains(QStringLiteral("DELETE")));
    CHECK(dropMilestone.contains(QStringLiteral("repos/{owner}/{repo}/milestones/3")));

    const QStringList pull = githubPullWriteArgv({}, QStringLiteral("Починил"), QStringLiteral("Готово"), QStringLiteral("main"),
                                                 QStringLiteral("fix"), true, &error);
    CHECK(error.isEmpty());
    CHECK(pull.contains(QStringLiteral("--head")));
    CHECK(pull.contains(QStringLiteral("fix")));
    CHECK(pull.contains(QStringLiteral("--draft")));
    CHECK(!pull.contains(QStringLiteral("--fill")));
    CHECK(githubPullWriteArgv({}, QStringLiteral("Починил"), {}, {}, {}, false, &error).isEmpty());
    const QStringList squash = githubPullMergeArgv(QStringLiteral("8"), QStringLiteral("squash"), &error);
    CHECK(squash.contains(QStringLiteral("--squash")));
    CHECK(!squash.contains(QStringLiteral("--delete-branch")));
    CHECK(!squash.contains(QStringLiteral("--admin")));
    CHECK(githubPullMergeArgv(QStringLiteral("8"), QStringLiteral("admin"), &error).isEmpty());
    CHECK(githubPullReviewArgv(QStringLiteral("8"), QStringLiteral("request"), {}, &error).isEmpty());
    CHECK(githubPullReadyArgv(QStringLiteral("8"), true, &error).contains(QStringLiteral("--undo")));
    CHECK(githubPullUpdateArgv(QStringLiteral("8"), true, &error).contains(QStringLiteral("--rebase")));

    const QStringList talk = githubDiscussionWriteArgv({}, QStringLiteral("Вопрос"), QStringLiteral("Как?"), QStringLiteral("Q&A"), &error);
    CHECK(talk.contains(QStringLiteral("--category")));
    CHECK(githubDiscussionCommentArgv(QStringLiteral("12"), {}, true, &error).isEmpty());
    const QStringList dropTalk = githubDiscussionCommentArgv(QStringLiteral("DC_abc123"), {}, true, &error);
    CHECK(dropTalk.contains(QStringLiteral("--delete")));
    CHECK(dropTalk.contains(QStringLiteral("--yes")));

    const QStringList release = githubReleaseCreateArgv(QStringLiteral("v1.2.3"), QStringLiteral("Правка"), QStringLiteral("заметки"),
                                                        true, false, QStringLiteral("main"), &error);
    CHECK(release.contains(QStringLiteral("--draft")));
    CHECK(release.contains(QStringLiteral("--target")));
    const QStringList editedRelease = githubReleaseEditArgv(QStringLiteral("v1.2.3"), QStringLiteral("Правка"), QStringLiteral("заметки"),
                                                            false, false, {}, &error);
    CHECK(editedRelease.contains(QStringLiteral("--draft=false")));
    const QStringList keepTag = githubReleaseDeleteArgv(QStringLiteral("v1.2.3"), false, &error);
    CHECK(keepTag.contains(QStringLiteral("--yes")));
    CHECK(!keepTag.contains(QStringLiteral("--cleanup-tag")));
    CHECK(githubReleaseDeleteArgv(QStringLiteral("v1.2.3"), true, &error).contains(QStringLiteral("--cleanup-tag")));
    CHECK(githubReleaseUploadArgv(QStringLiteral("v1.2.3"), QStringLiteral("/no/such/ghew-asset.txt"), &error).isEmpty());
    QTemporaryDir assetDir;
    CHECK(assetDir.isValid());
    const QString assetPath = assetDir.filePath(QStringLiteral("notes.txt"));
    QFile asset(assetPath);
    CHECK(asset.open(QIODevice::WriteOnly));
    CHECK(asset.write("hi") == 2);
    asset.close();
    const QStringList uploaded = githubReleaseUploadArgv(QStringLiteral("v1.2.3"), assetPath, &error);
    CHECK(error.isEmpty());
    CHECK(uploaded.contains(QFileInfo(assetPath).canonicalFilePath()));

    const QStringList link = githubAutolinkCreateArgv(QStringLiteral("TICKET-"),
                                                      QStringLiteral("https://example.com/TICKET?query=<num>"), true, &error);
    CHECK(error.isEmpty());
    CHECK(link.contains(QStringLiteral("--numeric")));
    CHECK(githubAutolinkCreateArgv(QStringLiteral("-TICKET"), QStringLiteral("https://example.com/<num>"), false, &error).isEmpty());
    CHECK(githubAutolinkCreateArgv(QStringLiteral("TICKET"), QStringLiteral("http://example.com/<num>"), false, &error).isEmpty());
    CHECK(githubAutolinkDeleteArgv(QStringLiteral("15"), &error).contains(QStringLiteral("--yes")));

    const QStringList field = githubProjectFieldArgv(QStringLiteral("1"), QStringLiteral("@me"), QStringLiteral("Статус"),
                                                     QStringLiteral("text"), {}, &error);
    CHECK(field.contains(QStringLiteral("TEXT")));
    CHECK(githubProjectFieldArgv(QStringLiteral("1"), QStringLiteral("@me"), QStringLiteral("Статус"),
                                 QStringLiteral("SINGLE_SELECT"), {}, &error)
              .isEmpty());
    const QStringList choice = githubProjectFieldArgv(QStringLiteral("1"), QStringLiteral("@me"), QStringLiteral("Статус"),
                                                      QStringLiteral("SINGLE_SELECT"), QStringLiteral("один, два"), &error);
    CHECK(choice.contains(QStringLiteral("один,два")));
    CHECK(githubProjectAddArgv(QStringLiteral("1"), QStringLiteral("@me"), QStringLiteral("https://evil.example/issues/1"), &error)
              .isEmpty());
    const QStringList card = githubProjectAddArgv(QStringLiteral("1"), QStringLiteral("@me"),
                                                  QStringLiteral("https://github.com/EmperyWorld/ghew/issues/4"), &error);
    CHECK(card.contains(QStringLiteral("--url")));
    const QStringList view = githubProjectViewArgv(QStringLiteral("PVT_test"), QStringLiteral("Доска"), QStringLiteral("BOARD_LAYOUT"), &error);
    CHECK(error.isEmpty());
    CHECK(view.join(QLatin1Char(' ')).contains(QStringLiteral("createProjectV2View")));
    CHECK(view.contains(QStringLiteral("layout=BOARD_LAYOUT")));
    CHECK(githubProjectViewArgv(QStringLiteral("PVT test"), QStringLiteral("Доска"), QStringLiteral("BOARD_LAYOUT"), &error).isEmpty());
    CHECK(githubProjectViewArgv(QStringLiteral("PVT_test"), QStringLiteral("Доска"), QStringLiteral("BOARD"), &error).isEmpty());
    CHECK(githubProjectLinkArgv(QStringLiteral("1"), QStringLiteral("@me"), &error).contains(QStringLiteral("link")));
    return 0;
}

static int testGithubAccess()
{
    QString error;
    const QStringList invite = githubCollaboratorArgv(QStringLiteral("octocat"), QStringLiteral("push"), false, &error);
    CHECK(error.isEmpty());
    CHECK(invite.contains(QStringLiteral("PUT")));
    CHECK(invite.contains(QStringLiteral("permission=push")));
    CHECK(githubCollaboratorArgv(QStringLiteral("../octocat"), QStringLiteral("admin"), false, &error).isEmpty());
    CHECK(githubCollaboratorArgv(QStringLiteral("octocat"), QStringLiteral("owner"), false, &error).isEmpty());
    CHECK(githubInvitationDeleteArgv(QStringLiteral("15"), &error).contains(QStringLiteral("DELETE")));
    const QStringList accepted = githubInvitationAcceptArgv(QStringLiteral("15"), &error);
    CHECK(error.isEmpty());
    CHECK(accepted.contains(QStringLiteral("PATCH")));
    CHECK(accepted.contains(QStringLiteral("user/repository_invitations/15")));
    CHECK(!accepted.join(QLatin1Char(' ')).contains(QStringLiteral("repos/")));

    const QStringList run = githubWorkflowArgv(QStringLiteral("run"), QStringLiteral("ci.yml"), QStringLiteral("main"),
                                               QStringLiteral("name"), QStringLiteral("ghew"), &error);
    CHECK(error.isEmpty());
    CHECK(run.contains(QStringLiteral("--ref")));
    CHECK(run.contains(QStringLiteral("name=ghew")));
    CHECK(!run.contains(QStringLiteral("--json")));
    CHECK(githubWorkflowArgv(QStringLiteral("enable"), QStringLiteral("-rf"), {}, {}, {}, &error).isEmpty());
    const QStringList dropRun = githubRunArgv(QStringLiteral("delete"), QStringLiteral("42"), false, &error);
    CHECK(dropRun.contains(QStringLiteral("42")));
    CHECK(!dropRun.contains(QStringLiteral("--all")));
    CHECK(githubRunArgv(QStringLiteral("rerun"), QStringLiteral("42"), true, &error).contains(QStringLiteral("--failed")));

    const QStringList variable = githubVariableArgv(QStringLiteral("REGION"), QStringLiteral("eu"), {}, false, &error);
    CHECK(variable.contains(QStringLiteral("--body")));
    CHECK(variable.contains(QStringLiteral("eu")));
    const GithubHiddenCommand secret = githubSecretCommand(QStringLiteral("TOKEN"), QStringLiteral("s3cret-value"), {},
                                                           QStringLiteral("actions"), false, &error);
    CHECK(error.isEmpty());
    CHECK(secret.stdinText == QStringLiteral("s3cret-value"));
    CHECK(!secret.argv.join(QLatin1Char(' ')).contains(QStringLiteral("s3cret-value")));
    CHECK(!secret.argv.contains(QStringLiteral("--body")));
    CHECK(githubSecretCommand(QStringLiteral("TOKEN"), QStringLiteral("s3cret-value"), {}, QStringLiteral("agents"), false, &error)
              .argv.isEmpty());
    CHECK(githubCacheDeleteArgv(QStringLiteral("--all"), &error).isEmpty());
    CHECK(githubCacheDeleteArgv(QStringLiteral("99"), &error).contains(QStringLiteral("99")));

    const QStringList dependabot = githubAlertArgv(QStringLiteral("dependabot"), QStringLiteral("7"), QStringLiteral("dismissed"),
                                                   QStringLiteral("tolerable_risk"), &error);
    CHECK(dependabot.contains(QStringLiteral("dismissed_reason=tolerable_risk")));
    CHECK(githubAlertArgv(QStringLiteral("dependabot"), QStringLiteral("7"), QStringLiteral("dismissed"),
                          QStringLiteral("false_positive"), &error)
              .isEmpty());
    const QStringList code = githubAlertArgv(QStringLiteral("code"), QStringLiteral("7"), QStringLiteral("dismissed"),
                                             QStringLiteral("false positive"), &error);
    CHECK(code.contains(QStringLiteral("dismissed_reason=false positive")));
    const QStringList leaked = githubAlertArgv(QStringLiteral("secret"), QStringLiteral("7"), QStringLiteral("resolved"),
                                               QStringLiteral("false_positive"), &error);
    CHECK(leaked.contains(QStringLiteral("resolution=false_positive")));

    const GithubHiddenCommand rules = githubRulesetCommand({}, QStringLiteral("main"), QStringLiteral("refs/heads/main"),
                                                           QStringLiteral("active"), QStringLiteral("deletion,non_fast_forward"),
                                                           &error);
    CHECK(error.isEmpty());
    CHECK(rules.argv.contains(QStringLiteral("POST")));
    CHECK(rules.stdinText.contains(QStringLiteral("deletion")));
    CHECK(rules.stdinText.contains(QStringLiteral("refs/heads/main")));
    CHECK(githubRulesetCommand({}, QStringLiteral("main"), QStringLiteral("refs/heads/../main"), QStringLiteral("active"),
                               QStringLiteral("deletion"), &error)
              .argv.isEmpty());

    const QStringList openBranch = githubBranchProtectionArgv(QStringLiteral("main"), 0, false, true, true, false, &error);
    CHECK(openBranch.contains(QStringLiteral("required_pull_request_reviews=null")));
    CHECK(openBranch.contains(QStringLiteral("allow_force_pushes=false")));
    const QStringList reviewed = githubBranchProtectionArgv(QStringLiteral("main"), 1, true, true, true, false, &error);
    CHECK(reviewed.contains(QStringLiteral("required_pull_request_reviews[required_approving_review_count]=1")));
    CHECK(githubBranchProtectionArgv(QStringLiteral("main"), 1, false, false, false, true, &error).contains(QStringLiteral("DELETE")));

    CHECK(githubDeployKeyArgv(QStringLiteral("ci"), QStringLiteral("-----BEGIN OPENSSH PRIVATE KEY-----\nabc"), true, &error)
              .isEmpty());
    const QStringList key = githubDeployKeyArgv(QStringLiteral("ci"), QStringLiteral("ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAItest"),
                                                true, &error);
    CHECK(error.isEmpty());
    CHECK(key.contains(QStringLiteral("read_only=true")));
    const GithubHiddenCommand hook = githubWebhookCommand({}, QStringLiteral("https://example.com/hook"), QStringLiteral("push"),
                                                          QStringLiteral("hook-secret"), true, false, &error);
    CHECK(error.isEmpty());
    CHECK(hook.stdinText.contains(QStringLiteral("hook-secret")));
    CHECK(!hook.argv.join(QLatin1Char(' ')).contains(QStringLiteral("hook-secret")));
    CHECK(githubWebhookCommand(QStringLiteral("3"), {}, {}, {}, false, true, &error).argv.contains(QStringLiteral("DELETE")));

    const QStringList advisory = githubAdvisoryArgv({}, QStringLiteral("Дыра"), QStringLiteral("Описание"), QStringLiteral("low"),
                                                    QStringLiteral("npm"), QStringLiteral("left-pad"), QStringLiteral("< 1.0.0"),
                                                    &error);
    CHECK(advisory.contains(QStringLiteral("vulnerabilities[][vulnerable_version_range]=< 1.0.0")));
    CHECK(!advisory.contains(QStringLiteral("state=published")));
    CHECK(githubInteractionArgv({}, true, &error).contains(QStringLiteral("DELETE")));

    const QStringList space = githubCodespaceCreateArgv(QStringLiteral("EmperyWorld/ghew"), QStringLiteral("main"),
                                                        QStringLiteral("Черновик"), &error);
    CHECK(error.isEmpty());
    CHECK(space.contains(QStringLiteral("--default-permissions")));
    CHECK(space.contains(QStringLiteral("--repo")));
    CHECK(!space.contains(QStringLiteral("--web")));
    CHECK(!space.contains(QStringLiteral("--all")));
    const QStringList gone = githubCodespaceDeleteArgv(QStringLiteral("redesigned-goldfish"), &error);
    CHECK(gone.contains(QStringLiteral("DELETE")));
    CHECK(gone.contains(QStringLiteral("user/codespaces/redesigned-goldfish")));
    CHECK(githubCodespaceDeleteArgv(QStringLiteral("--all"), &error).isEmpty());

    const QStringList env = githubEnvironmentArgv(QStringLiteral("prod"), 15, false, QStringLiteral("User:12,Team:34"), false,
                                                  &error);
    CHECK(error.isEmpty());
    CHECK(env.contains(QStringLiteral("reviewers[][type]=User")));
    CHECK(env.contains(QStringLiteral("reviewers[][id]=34")));
    CHECK(githubEnvironmentArgv(QStringLiteral("prod"), 15, false, QStringLiteral("Bot:1"), false, &error).isEmpty());
    CHECK(githubEnvironmentArgv(QStringLiteral("prod"), 15, false,
                                QStringLiteral("User:1,User:2,User:3,User:4,User:5,User:6,User:7"), false, &error)
              .isEmpty());

    const QStringList renamed = githubProjectViewEditArgv(QStringLiteral("PVT_abc"), QStringLiteral("PVTV_view"),
                                                          QStringLiteral("Доска"), false, &error);
    CHECK(error.isEmpty());
    CHECK(renamed.join(QLatin1Char(' ')).contains(QStringLiteral("updateProjectV2View")));
    const QStringList dropped = githubProjectViewEditArgv(QStringLiteral("PVT_abc"), QStringLiteral("PVTV_view"), {}, true, &error);
    CHECK(dropped.join(QLatin1Char(' ')).contains(QStringLiteral("deleteProjectV2View")));
    CHECK(githubProjectViewEditArgv(QStringLiteral("PVT_abc"), QStringLiteral("PVT_view"), QStringLiteral("Доска"), false, &error)
              .isEmpty());
    return 0;
}

static int testGithubIdentity()
{
    QString error;
    const GithubHiddenCommand profile = githubProfileCommand(QStringLiteral("Octo"), QStringLiteral("Пишет код"),
                                                             QStringLiteral("https://example.com"), QStringLiteral("Empery"),
                                                             QStringLiteral("Vladivostok"), QStringLiteral("@octocat"), true, &error);
    CHECK(error.isEmpty());
    CHECK(profile.argv.contains(QStringLiteral("PATCH")));
    CHECK(profile.argv.contains(QStringLiteral("user")));
    CHECK(profile.stdinText.contains(QStringLiteral("\"hireable\":true")));
    CHECK(profile.stdinText.contains(QStringLiteral("octocat")));
    CHECK(!profile.argv.join(QLatin1Char(' ')).contains(QStringLiteral("Пишет код")));
    CHECK(githubProfileCommand(QStringLiteral("Octo"), QStringLiteral("bio"), QStringLiteral("example.com"), {}, {}, {}, false, &error)
              .argv.isEmpty());

    const GithubHiddenCommand mail = githubEmailCommand(QStringLiteral("dev@example.com"), QStringLiteral("delete"), &error);
    CHECK(error.isEmpty());
    CHECK(mail.argv.contains(QStringLiteral("DELETE")));
    CHECK(mail.stdinText.contains(QStringLiteral("dev@example.com")));
    CHECK(!mail.argv.join(QLatin1Char(' ')).contains(QStringLiteral("dev@example.com")));
    CHECK(githubEmailCommand({}, QStringLiteral("public"), &error).stdinText.contains(QStringLiteral("public")));
    CHECK(githubEmailCommand(QStringLiteral("not-an-email"), QStringLiteral("add"), &error).argv.isEmpty());
    const QByteArray emailJson =
        QByteArrayLiteral("[{\"email\":\"a@b.co\",\"primary\":true,\"verified\":true,\"visibility\":\"public\"},"
                          "{\"email\":\"c@d.co\",\"primary\":false,\"verified\":false,\"visibility\":null}]");
    const QStringList listed = githubEmailLines(emailJson, &error);
    CHECK(error.isEmpty());
    CHECK(listed.size() == 2);
    CHECK(listed.at(0).contains(QStringLiteral("a@b.co")));
    CHECK(listed.at(0).contains(QStringLiteral("основная")));
    CHECK(listed.at(0).contains(QStringLiteral("видна всем")));
    CHECK(listed.at(1).contains(QStringLiteral("скрыта")));
    CHECK(listed.at(1).contains(QStringLiteral("не подтверждена")));
    CHECK(githubEmailLines(QByteArrayLiteral("{\"email\":\"a@b.co\"}"), &error).isEmpty());

    const GithubHiddenCommand social = githubSocialCommand(QStringLiteral("https://www.linkedin.com/in/octo"), false, &error);
    CHECK(error.isEmpty());
    CHECK(social.argv.contains(QStringLiteral("POST")));
    CHECK(social.stdinText.contains(QStringLiteral("account_urls")));
    CHECK(githubSocialCommand(QStringLiteral("http://example.com"), false, &error).argv.isEmpty());

    CHECK(githubUserKeyCommand(QStringLiteral("ssh"), QStringLiteral("laptop"),
                                QStringLiteral("-----BEGIN OPENSSH PRIVATE KEY-----\nabc"), {}, false, &error)
              .argv.isEmpty());
    const GithubHiddenCommand ssh = githubUserKeyCommand(QStringLiteral("ssh"), QStringLiteral("laptop"),
                                                         QStringLiteral("ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAItest"), {}, false,
                                                         &error);
    CHECK(error.isEmpty());
    CHECK(ssh.stdinText.contains(QStringLiteral("ssh-ed25519")));
    CHECK(!ssh.argv.join(QLatin1Char(' ')).contains(QStringLiteral("ssh-ed25519")));
    const GithubHiddenCommand gpg = githubUserKeyCommand(QStringLiteral("gpg"), QStringLiteral("pack"),
                                                         QStringLiteral("-----BEGIN PGP PUBLIC KEY BLOCK-----\nabc"), {}, false,
                                                         &error);
    CHECK(error.isEmpty());
    CHECK(gpg.argv.contains(QStringLiteral("user/gpg_keys")));
    CHECK(gpg.stdinText.contains(QStringLiteral("BEGIN PGP PUBLIC KEY BLOCK")));
    CHECK(!gpg.argv.join(QLatin1Char(' ')).contains(QStringLiteral("BEGIN PGP PUBLIC KEY BLOCK")));
    CHECK(githubUserKeyCommand(QStringLiteral("ssh"), {}, {}, QStringLiteral("4"), true, &error).argv.contains(QStringLiteral("user/keys/4")));

    const QStringList created = githubProfileRepoArgv(QStringLiteral("octocat"), &error);
    CHECK(error.isEmpty());
    CHECK(created.contains(QStringLiteral("--public")));
    CHECK(created.contains(QStringLiteral("--add-readme")));
    CHECK(created.contains(QStringLiteral("octocat")));
    CHECK(!created.contains(QStringLiteral("--clone")));
    CHECK(!created.contains(QStringLiteral("--private")));
    CHECK(githubProfileRepoArgv(QStringLiteral("octo/cat"), &error).isEmpty());
    const GithubHiddenCommand readme = githubProfileReadmeCommand(QStringLiteral("octocat"), QStringLiteral("# Hello"),
                                                                  QStringLiteral("0123456789abcdef0123456789abcdef01234567"),
                                                                  &error);
    CHECK(error.isEmpty());
    CHECK(readme.argv.contains(QStringLiteral("repos/octocat/octocat/contents/README.md")));
    CHECK(readme.stdinText.contains(QStringLiteral("0123456789abcdef0123456789abcdef01234567")));
    CHECK(!readme.argv.join(QLatin1Char(' ')).contains(QStringLiteral("# Hello")));

    const GithubHiddenCommand card = githubOrgCardCommand(QStringLiteral("acme"), QStringLiteral("Acme"), QStringLiteral("Делаем"),
                                                          QStringLiteral("https://acme.example"), QStringLiteral("hi@acme.example"),
                                                          QStringLiteral("acme"), QStringLiteral("Town"), QStringLiteral("Acme Inc"),
                                                          &error);
    CHECK(error.isEmpty());
    CHECK(card.argv.contains(QStringLiteral("orgs/acme")));
    CHECK(card.stdinText.contains(QStringLiteral("Делаем")));
    CHECK(githubOrgCardCommand(QStringLiteral("acme"), {}, {}, {}, {}, {}, {}, {}, &error).argv.isEmpty());

    const GithubHiddenCommand policy = githubOrgPolicyCommand(QStringLiteral("acme"), QStringLiteral("read"), true, false, false, true,
                                                              true, false, false, &error);
    CHECK(error.isEmpty());
    CHECK(policy.stdinText.contains(QStringLiteral("members_can_create_internal_repositories\":false")));
    CHECK(policy.stdinText.contains(QStringLiteral("members_can_fork_private_repositories\":false")));
    CHECK(githubOrgPolicyCommand(QStringLiteral("acme"), QStringLiteral("owner"), true, true, false, true, true, true, false, &error)
              .argv.isEmpty());

    const GithubHiddenCommand team = githubOrgTeamCommand(QStringLiteral("acme"), QStringLiteral("Platform"), {}, QStringLiteral("secret"),
                                                          {}, false, &error);
    CHECK(error.isEmpty());
    CHECK(team.argv.contains(QStringLiteral("orgs/acme/teams")));
    CHECK(team.stdinText.contains(QStringLiteral("secret")));
    CHECK(githubOrgTeamCommand(QStringLiteral("acme"), {}, {}, {}, QStringLiteral("platform"), true, &error)
              .argv.contains(QStringLiteral("DELETE")));
    CHECK(githubOrgTeamMemberCommand(QStringLiteral("acme"), QStringLiteral("platform"), QStringLiteral("octocat"),
                                     QStringLiteral("maintainer"), false, &error)
              .stdinText.contains(QStringLiteral("maintainer")));
    CHECK(githubOrgPeopleCommand(QStringLiteral("acme"), QStringLiteral("octocat"), {}, {}, {}, QStringLiteral("remove-member"), &error)
              .argv.contains(QStringLiteral("orgs/acme/members/octocat")));
    CHECK(githubOrgPeopleCommand(QStringLiteral("acme"), {}, QStringLiteral("new@acme.example"), {}, QStringLiteral("admin"),
                                 QStringLiteral("invite"), &error)
              .stdinText.contains(QStringLiteral("admin")));
    CHECK(githubOrgPeopleCommand(QStringLiteral("acme"), {}, {}, {}, {}, QStringLiteral("remove-outside"), &error).argv.isEmpty());

    const GithubHiddenCommand role = githubOrgRoleCommand(QStringLiteral("acme"), {}, QStringLiteral("Labeler"), {},
                                                          QStringLiteral("read"), QStringLiteral("add_label,manage_wiki"), false, &error);
    CHECK(error.isEmpty());
    CHECK(role.argv.contains(QStringLiteral("POST")));
    CHECK(role.stdinText.contains(QStringLiteral("manage_wiki")));
    CHECK(githubOrgRoleCommand(QStringLiteral("acme"), {}, QStringLiteral("Labeler"), {}, QStringLiteral("read"),
                               QStringLiteral("admin"), false, &error)
              .argv.isEmpty());
    CHECK(githubOrgRoleCommand(QStringLiteral("acme"), QStringLiteral("9"), {}, {}, {}, {}, true, &error)
              .argv.contains(QStringLiteral("orgs/acme/custom-repository-roles/9")));

    const GithubHiddenCommand property = githubOrgPropertyCommand(QStringLiteral("acme"), QStringLiteral("environment"),
                                                                  QStringLiteral("single_select"), {}, QStringLiteral("prod,dev"),
                                                                  false, &error);
    CHECK(error.isEmpty());
    CHECK(property.stdinText.contains(QStringLiteral("allowed_values")));
    CHECK(property.stdinText.contains(QStringLiteral("org_actors")));
    CHECK(githubOrgPropertyCommand(QStringLiteral("acme"), QStringLiteral("Environment"), QStringLiteral("string"), {}, {}, false, &error)
              .argv.isEmpty());

    const GithubHiddenCommand hook = githubOrgHookCommand(QStringLiteral("acme"), {}, QStringLiteral("https://example.com/hook"),
                                                          QStringLiteral("push"), QStringLiteral("hook-secret"), true, false, &error);
    CHECK(error.isEmpty());
    CHECK(hook.stdinText.contains(QStringLiteral("hook-secret")));
    CHECK(!hook.argv.join(QLatin1Char(' ')).contains(QStringLiteral("hook-secret")));
    CHECK(hook.argv.contains(QStringLiteral("orgs/acme/hooks")));

    const GithubHiddenCommand config = githubOrgSecurityConfigCommand(QStringLiteral("acme"), QStringLiteral("Baseline"),
                                                                      QStringLiteral("enabled"), QStringLiteral("not_set"),
                                                                      QStringLiteral("disabled"), {}, false, &error);
    CHECK(error.isEmpty());
    CHECK(config.stdinText.contains(QStringLiteral("dependabot_alerts")));
    CHECK(!config.stdinText.contains(QStringLiteral("advanced_security")));
    CHECK(config.argv.contains(QStringLiteral("POST")));
    CHECK(githubOrgSecurityConfigCommand(QStringLiteral("acme"), {}, {}, {}, {}, QStringLiteral("3"), true, &error)
              .argv.contains(QStringLiteral("orgs/acme/code-security/configurations/3")));
    CHECK(githubOrgSecurityManagerArgv(QStringLiteral("acme"), QStringLiteral("security"), false, &error)
              .contains(QStringLiteral("orgs/acme/security-managers/teams/security")));
    const GithubHiddenCommand frozen = githubOrgImmutableCommand(QStringLiteral("acme"), QStringLiteral("all"), {}, &error);
    CHECK(error.isEmpty());
    CHECK(frozen.stdinText.contains(QStringLiteral("\"enforced_repositories\":\"all\"")));
    CHECK(githubOrgImmutableCommand(QStringLiteral("acme"), QStringLiteral("selected"), {}, &error).argv.isEmpty());
    CHECK(githubOrgImmutableCommand(QStringLiteral("acme"), QStringLiteral("all"), QStringLiteral("32"), &error).argv.isEmpty());
    const GithubHiddenCommand picked = githubOrgImmutableCommand(QStringLiteral("acme"), QStringLiteral("selected"),
                                                                 QStringLiteral("32,91"), &error);
    CHECK(error.isEmpty());
    CHECK(picked.stdinText.contains(QStringLiteral("\"selected_repository_ids\":[32,91]")));

    const GithubHiddenCommand banner = githubOrgBannerCommand(QStringLiteral("acme"), QStringLiteral("Важно"),
                                                              QStringLiteral("2026-12-01T00:00:00Z"), true, false, &error);
    CHECK(error.isEmpty());
    CHECK(banner.stdinText.contains(QStringLiteral("2026-12-01T00:00:00Z")));
    CHECK(githubOrgBannerCommand(QStringLiteral("acme"), QStringLiteral("Важно"), QStringLiteral("завтра"), true, false, &error)
              .argv.isEmpty());

    const QStringList assigned = githubOrgRoleAssignArgv(QStringLiteral("acme"), QStringLiteral("octocat"), QStringLiteral("4"),
                                                         false, &error);
    CHECK(error.isEmpty());
    CHECK(assigned.contains(QStringLiteral("PUT")));
    CHECK(assigned.contains(QStringLiteral("orgs/acme/organization-roles/users/octocat/4")));
    CHECK(githubOrgRoleAssignArgv(QStringLiteral("acme"), QStringLiteral("octocat"), QStringLiteral("4"), true, &error)
              .contains(QStringLiteral("DELETE")));

    const GithubHiddenCommand attach = githubOrgSecurityAttachCommand(QStringLiteral("acme"), QStringLiteral("3"),
                                                                      QStringLiteral("32,91"), &error);
    CHECK(error.isEmpty());
    CHECK(attach.stdinText.contains(QStringLiteral("\"scope\":\"selected\"")));
    CHECK(attach.stdinText.contains(QStringLiteral("[32,91]")));
    CHECK(!attach.stdinText.contains(QStringLiteral("\"scope\":\"all\"")));
    CHECK(githubOrgSecurityAttachCommand(QStringLiteral("acme"), QStringLiteral("3"), QStringLiteral("all"), &error).argv.isEmpty());
    return 0;
}

static int testGithubDeskActions()
{
    CHECK(githubDeskActionCount() >= 70);
    CHECK(githubDeskActionKnown(QStringLiteral("gh_issue")));
    CHECK(githubDeskActionKnown(QStringLiteral("gh_emails")));
    CHECK(githubDeskActionKnown(QStringLiteral("gh_invitation_accept")));
    CHECK(githubDeskActionKnown(QStringLiteral("gh_org_role_assign")));
    CHECK(!githubDeskActionKnown(QStringLiteral("github_create")));
    CHECK(githubDeskActionTitle(QStringLiteral("gh_wiki")).contains(QStringLiteral("wiki")));
    const QString catalog = githubDeskActionCatalog();
    CHECK(catalog.contains(QStringLiteral("gh_secret_delete")));
    CHECK(catalog.contains(QStringLiteral("gh_template")));
    CHECK(catalog.contains(QStringLiteral("gh_services")));
    CHECK(catalog.contains(QStringLiteral("gh_wiki_list")));
    CHECK(!catalog.contains(QStringLiteral("gh_secret ")));
    QVariantMap secret{{QStringLiteral("name"), QStringLiteral("TOKEN")}, {QStringLiteral("value"), QStringLiteral("sekret")}};
    CHECK(!githubDeskAcceptAdvisorCommand(QStringLiteral("gh_secret_delete"), &secret));
    secret.remove(QStringLiteral("value"));
    CHECK(githubDeskAcceptAdvisorCommand(QStringLiteral("gh_secret_delete"), &secret));
    QVariantMap hook{{QStringLiteral("url"), QStringLiteral("https://example.com/hook")},
                     {QStringLiteral("secret"), QStringLiteral("hidden")}};
    CHECK(!githubDeskAcceptAdvisorCommand(QStringLiteral("gh_webhook"), &hook));
    CHECK(!githubDeskAcceptAdvisorCommand(QStringLiteral("gh_org_hook"), &hook));
    hook.remove(QStringLiteral("secret"));
    CHECK(githubDeskAcceptAdvisorCommand(QStringLiteral("gh_webhook"), &hook));
    QVariantMap key{{QStringLiteral("key"), QStringLiteral("-----BEGIN PRIVATE KEY-----")}};
    CHECK(!githubDeskAcceptAdvisorCommand(QStringLiteral("gh_user_key"), &key));
    CHECK(!githubDeskAcceptAdvisorCommand(QStringLiteral("gh_deploy_key"), &key));
    key.insert(QStringLiteral("key"), QStringLiteral("ssh-ed25519 AAAA"));
    CHECK(githubDeskAcceptAdvisorCommand(QStringLiteral("gh_deploy_key"), &key));
    QStringList ids = catalog.split(QLatin1Char('\n'));
    ids.removeAll(QString());
    CHECK(ids.size() == githubDeskActionCount());
    return 0;
}

int testLlmLive();

int main()
{
    if (const int rc = testPorcelain())
        return rc;
    if (const int rc = testPreviewSafe())
        return rc;
    if (const int rc = testHunks())
        return rc;
    if (const int rc = testCommitFilter())
        return rc;
    if (const int rc = testMentorReply())
        return rc;
    if (const int rc = testEphemeralRecent())
        return rc;
    if (const int rc = testGithubRemote())
        return rc;
    if (const int rc = testGithubLifecycle())
        return rc;
    if (const int rc = testGithubServices())
        return rc;
    if (const int rc = testGithubAccess())
        return rc;
    if (const int rc = testGithubIdentity())
        return rc;
    if (const int rc = testGithubDeskActions())
        return rc;
    if (const int rc = testLlmLive())
        return rc;
    std::printf("UNIT_OK\n");
    return 0;
}
