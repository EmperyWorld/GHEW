#include "GithubScaffold.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStringList>

namespace {

bool anyExists(const QString &root, const QStringList &names)
{
    for (const QString &name : names) {
        if (QFileInfo::exists(root + QLatin1Char('/') + name))
            return true;
    }
    return false;
}

bool writeUtf8(const QString &path, const QString &body, QString *error)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::NewOnly | QIODevice::Text)) {
        if (error)
            *error = QStringLiteral("Не удалось создать %1").arg(path);
        return false;
    }
    file.write(body.toUtf8());
    return true;
}

QString mitText(int year, const QString &owner)
{
    return QStringLiteral(
               "MIT License\n"
               "\n"
               "Copyright (c) %1 %2\n"
               "\n"
               "Permission is hereby granted, free of charge, to any person obtaining a copy\n"
               "of this software and associated documentation files (the \"Software\"), to deal\n"
               "in the Software without restriction, including without limitation the rights\n"
               "to use, copy, modify, merge, publish, distribute, sublicense, and/or sell\n"
               "copies of the Software, and to permit persons to whom the Software is\n"
               "furnished to do so, subject to the following conditions:\n"
               "\n"
               "The above copyright notice and this permission notice shall be included in all\n"
               "copies or substantial portions of the Software.\n"
               "\n"
               "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\n"
               "IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\n"
               "FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\n"
               "AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER\n"
               "LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,\n"
               "OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE\n"
               "SOFTWARE.\n")
        .arg(year)
        .arg(owner);
}

QString bsdText(int year, const QString &owner)
{
    return QStringLiteral(
               "BSD 2-Clause License\n"
               "\n"
               "Copyright (c) %1 %2\n"
               "\n"
               "Redistribution and use in source and binary forms, with or without\n"
               "modification, are permitted provided that the following conditions are met:\n"
               "\n"
               "1. Redistributions of source code must retain the above copyright notice, this\n"
               "   list of conditions and the following disclaimer.\n"
               "\n"
               "2. Redistributions in binary form must reproduce the above copyright notice,\n"
               "   this list of conditions and the following disclaimer in the documentation\n"
               "   and/or other materials provided with the distribution.\n"
               "\n"
               "THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\"\n"
               "AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE\n"
               "IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE\n"
               "DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE\n"
               "FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL\n"
               "DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR\n"
               "SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER\n"
               "CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,\n"
               "OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE\n"
               "OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n")
        .arg(year)
        .arg(owner);
}

QString proprietaryText(int year, const QString &owner)
{
    return QStringLiteral(
               "Copyright (c) %1 %2\n"
               "\n"
               "All rights reserved.\n"
               "\n"
               "This repository and its contents are proprietary. No permission is granted to\n"
               "use, copy, modify, merge, publish, distribute, sublicense, or sell copies\n"
               "without prior written consent of the copyright holder.\n")
        .arg(year)
        .arg(owner);
}

} // namespace

bool writeScaffoldReadme(const QString &repoRoot, const QString &title, const QString &blurb, QString *error)
{
    if (repoRoot.isEmpty() || !QDir(repoRoot).exists()) {
        if (error)
            *error = QStringLiteral("Каталог репозитория недоступен.");
        return false;
    }
    if (anyExists(repoRoot, {QStringLiteral("README.md"), QStringLiteral("README"), QStringLiteral("readme.md")})) {
        if (error)
            *error = QStringLiteral("README уже есть — существующий файл не перезаписывается.");
        return false;
    }
    const QString heading = title.trimmed().isEmpty() ? QFileInfo(repoRoot).fileName() : title.trimmed();
    return writeUtf8(repoRoot + QStringLiteral("/README.md"), scaffoldReadmeText(heading, blurb), error);
}

bool writeScaffoldLicense(const QString &repoRoot, const QString &kind, const QString &owner, int year,
                          QString *error)
{
    if (repoRoot.isEmpty() || !QDir(repoRoot).exists()) {
        if (error)
            *error = QStringLiteral("Каталог репозитория недоступен.");
        return false;
    }
    if (anyExists(repoRoot,
                  {QStringLiteral("LICENSE"), QStringLiteral("LICENSE.md"), QStringLiteral("COPYING"),
                   QStringLiteral("LICENCE"), QStringLiteral("LICENCE.md")})) {
        if (error)
            *error = QStringLiteral("Файл лицензии уже есть — он не перезаписывается.");
        return false;
    }
    const QString body = scaffoldLicenseText(kind, owner, year);
    if (body.isEmpty()) {
        if (error)
            *error = QStringLiteral("Неизвестный вид лицензии: %1. Допустимо: mit, bsd2, proprietary.").arg(kind);
        return false;
    }
    return writeUtf8(repoRoot + QStringLiteral("/LICENSE"), body, error);
}

QString scaffoldReadmeText(const QString &title, const QString &blurb)
{
    const QString heading = title.trimmed().isEmpty() ? QStringLiteral("Репозиторий") : title.trimmed();
    const QString about = blurb.trimmed().isEmpty()
                              ? QStringLiteral("Кратко опишите, зачем нужен этот проект.")
                              : blurb.trimmed();
    return QStringLiteral("# %1\n\n%2\n\n## Назначение\n\n%3\n\n## Как запустить\n\n"
                          "Добавьте команды сборки и запуска.\n")
        .arg(heading, about, about);
}

QString scaffoldLicenseText(const QString &kind, const QString &owner, int year)
{
    const QString who = owner.trimmed().isEmpty() ? QStringLiteral("The authors") : owner.trimmed();
    const QString key = kind.trimmed().toLower();
    if (key == QLatin1String("mit"))
        return mitText(year, who);
    if (key == QLatin1String("bsd2"))
        return bsdText(year, who);
    if (key == QLatin1String("proprietary"))
        return proprietaryText(year, who);
    return {};
}

namespace {

QString existingOr(const QString &root, const QStringList &names, const QString &fallback)
{
    for (const QString &name : names) {
        if (QFileInfo::exists(root + QLatin1Char('/') + name))
            return name;
    }
    return fallback;
}

bool pathInside(const QString &root, const QString &filePath)
{
    const QString rootCanon = QFileInfo(root).canonicalFilePath();
    if (rootCanon.isEmpty())
        return false;
    const QString relative = QDir(rootCanon).relativeFilePath(QFileInfo(filePath).absoluteFilePath());
    if (relative.isEmpty() || relative.startsWith(QLatin1String("..")) || relative.startsWith(QLatin1Char('/')))
        return false;
    QString walk = rootCanon;
    const QStringList parts = relative.split(QLatin1Char('/'), Qt::SkipEmptyParts);
    for (int i = 0; i < parts.size(); ++i) {
        const QString part = parts.at(i);
        if (part == QLatin1String(".") || part == QLatin1String(".."))
            return false;
        const QString next = walk + QLatin1Char('/') + part;
        const QFileInfo info(next);
        if (!info.exists())
            return true;
        const QString canon = info.canonicalFilePath();
        if (canon != rootCanon && !canon.startsWith(rootCanon + QLatin1Char('/')))
            return false;
        walk = canon;
        if (i + 1 == parts.size() && info.isSymLink()) {
            const QString target = info.canonicalFilePath();
            if (target != rootCanon && !target.startsWith(rootCanon + QLatin1Char('/')))
                return false;
        }
    }
    return true;
}

} // namespace

QStringList dressKinds()
{
    return {QStringLiteral("readme"),      QStringLiteral("license"),     QStringLiteral("gitignore"),
            QStringLiteral("contributing"), QStringLiteral("conduct"),    QStringLiteral("security"),
            QStringLiteral("support"),     QStringLiteral("funding"),     QStringLiteral("codeowners"),
            QStringLiteral("issue"),       QStringLiteral("pull")};
}

bool dressFilePresent(const QString &repoRoot, const QString &kind)
{
    if (repoRoot.isEmpty())
        return false;
    const QString name = dressFileName(repoRoot, kind);
    if (name.isEmpty())
        return false;
    return QFileInfo::exists(repoRoot + QLatin1Char('/') + name);
}

QString dressFileName(const QString &repoRoot, const QString &kind)
{
    const QString key = kind.trimmed().toLower();
    if (key == QLatin1String("readme"))
        return existingOr(repoRoot,
                          {QStringLiteral("README.md"), QStringLiteral("README"), QStringLiteral("readme.md")},
                          QStringLiteral("README.md"));
    if (key == QLatin1String("license"))
        return existingOr(repoRoot,
                          {QStringLiteral("LICENSE"), QStringLiteral("LICENSE.md"), QStringLiteral("COPYING"),
                           QStringLiteral("LICENCE"), QStringLiteral("LICENCE.md")},
                          QStringLiteral("LICENSE"));
    if (key == QLatin1String("gitignore"))
        return QStringLiteral(".gitignore");
    if (key == QLatin1String("contributing"))
        return existingOr(repoRoot,
                          {QStringLiteral("CONTRIBUTING.md"), QStringLiteral(".github/CONTRIBUTING.md"),
                           QStringLiteral("docs/CONTRIBUTING.md")},
                          QStringLiteral("CONTRIBUTING.md"));
    if (key == QLatin1String("conduct"))
        return existingOr(repoRoot,
                          {QStringLiteral("CODE_OF_CONDUCT.md"), QStringLiteral(".github/CODE_OF_CONDUCT.md")},
                          QStringLiteral("CODE_OF_CONDUCT.md"));
    if (key == QLatin1String("security"))
        return existingOr(repoRoot, {QStringLiteral("SECURITY.md"), QStringLiteral(".github/SECURITY.md")},
                          QStringLiteral("SECURITY.md"));
    if (key == QLatin1String("support"))
        return existingOr(repoRoot, {QStringLiteral("SUPPORT.md"), QStringLiteral(".github/SUPPORT.md")},
                          QStringLiteral("SUPPORT.md"));
    if (key == QLatin1String("funding"))
        return existingOr(repoRoot, {QStringLiteral(".github/FUNDING.yml"), QStringLiteral("FUNDING.yml")},
                          QStringLiteral(".github/FUNDING.yml"));
    if (key == QLatin1String("codeowners"))
        return existingOr(repoRoot,
                          {QStringLiteral(".github/CODEOWNERS"), QStringLiteral("CODEOWNERS"),
                           QStringLiteral("docs/CODEOWNERS")},
                          QStringLiteral(".github/CODEOWNERS"));
    if (key == QLatin1String("issue"))
        return existingOr(repoRoot,
                          {QStringLiteral(".github/ISSUE_TEMPLATE.md"),
                           QStringLiteral(".github/ISSUE_TEMPLATE/bug.md")},
                          QStringLiteral(".github/ISSUE_TEMPLATE/bug.md"));
    if (key == QLatin1String("pull"))
        return existingOr(repoRoot,
                          {QStringLiteral(".github/PULL_REQUEST_TEMPLATE.md"),
                           QStringLiteral("PULL_REQUEST_TEMPLATE.md")},
                          QStringLiteral(".github/PULL_REQUEST_TEMPLATE.md"));
    return {};
}

QString dressTemplateText(const QString &kind, const QString &title)
{
    const QString heading = title.trimmed().isEmpty() ? QStringLiteral("Проект") : title.trimmed();
    const QString key = kind.trimmed().toLower();
    if (key == QLatin1String("contributing"))
        return QStringLiteral("# Участие в %1\n\nОпишите, как предложить изменение: ветка, проверка, pull request.\n").arg(heading);
    if (key == QLatin1String("conduct"))
        return QStringLiteral("# Правила поведения\n\nУчастники %1 общаются уважительно. Оскорбления и преследование недопустимы.\n")
            .arg(heading);
    if (key == QLatin1String("security"))
        return QStringLiteral("# Безопасность\n\nСообщайте об уязвимостях в %1 приватно, не через публичный issue.\n").arg(heading);
    if (key == QLatin1String("support"))
        return QStringLiteral("# Поддержка\n\nВопросы по %1 задавайте через issue. Срочные сбои опишите отдельно от предложений.\n")
            .arg(heading);
    if (key == QLatin1String("funding"))
        return QStringLiteral("# Укажите площадки поддержки. Пример:\n# github: [owner]\n");
    if (key == QLatin1String("codeowners"))
        return QStringLiteral("# Путь    @владелец\n* @owner\n");
    if (key == QLatin1String("issue"))
        return QStringLiteral("---\nname: Ошибка\nabout: Что сломалось\ntitle: \"\"\nlabels: bug\n---\n\n## Что произошло\n\n## Как повторить\n\n");
    if (key == QLatin1String("pull"))
        return QStringLiteral("## Зачем\n\n## Что изменено\n\n## Как проверить\n");
    return {};
}

QString readDressFile(const QString &repoRoot, const QString &kind)
{
    const QString name = dressFileName(repoRoot, kind);
    if (name.isEmpty() || repoRoot.isEmpty())
        return {};
    const QString path = repoRoot + QLatin1Char('/') + name;
    if (!pathInside(repoRoot, path) && QFileInfo::exists(path))
        return {};
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    return QString::fromUtf8(file.readAll());
}

bool writeDressFile(const QString &repoRoot, const QString &kind, const QString &body, QString *error)
{
    if (repoRoot.isEmpty() || !QDir(repoRoot).exists()) {
        if (error)
            *error = QStringLiteral("Каталог репозитория недоступен.");
        return false;
    }
    const QString name = dressFileName(repoRoot, kind);
    if (name.isEmpty() || name.contains(QLatin1String("..")) || name.startsWith(QLatin1Char('/'))) {
        if (error)
            *error = QStringLiteral("Этот вид файла править нельзя.");
        return false;
    }
    if (body.trimmed().isEmpty()) {
        if (error)
            *error = QStringLiteral("Пустой текст — файл не записан. Нужен body или шаблон.");
        return false;
    }
    if (body.size() > 1000000) {
        if (error)
            *error = QStringLiteral("Текст длиннее 1 МБ — он не записан.");
        return false;
    }
    const QString path = repoRoot + QLatin1Char('/') + name;
    if (!pathInside(repoRoot, path)) {
        if (error)
            *error = QStringLiteral("Файл вне каталога репозитория — запись отменена.");
        return false;
    }
    const QString parent = QFileInfo(path).absolutePath();
    if (!QDir().mkpath(parent)) {
        if (error)
            *error = QStringLiteral("Не удалось создать каталог для %1").arg(name);
        return false;
    }
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        if (error)
            *error = QStringLiteral("Не удалось записать %1").arg(name);
        return false;
    }
    file.write(body.toUtf8());
    if (!body.endsWith(QLatin1Char('\n')))
        file.write("\n");
    return true;
}
