#include "ReleaseNotes.h"

#include <QCoreApplication>
#include <QRegularExpression>
#include <QStringList>

#include <array>
#include <utility>

namespace {

constexpr auto kContext = "update::ReleaseNotes";
constexpr auto kFallbackIcon = "notes";

struct Group
{
    QLatin1StringView key;
    const char *title;
    const char *icon;
};

const std::array kGroups {
    Group {QLatin1StringView("features"), QT_TRANSLATE_NOOP("update::ReleaseNotes", "Features"),
           "auto_awesome"},
    Group {QLatin1StringView("improvements"),
           QT_TRANSLATE_NOOP("update::ReleaseNotes", "Improvements"), "trending_up"},
    Group {QLatin1StringView("bug fixes"), QT_TRANSLATE_NOOP("update::ReleaseNotes", "Bug fixes"),
           "bug_report"},
    Group {QLatin1StringView("performance"),
           QT_TRANSLATE_NOOP("update::ReleaseNotes", "Performance"), "speed"},
    Group {QLatin1StringView("security"), QT_TRANSLATE_NOOP("update::ReleaseNotes", "Security"),
           "shield"},
    Group {QLatin1StringView("documentation"),
           QT_TRANSLATE_NOOP("update::ReleaseNotes", "Documentation"), "menu_book"},
    Group {QLatin1StringView("refactoring"),
           QT_TRANSLATE_NOOP("update::ReleaseNotes", "Refactoring"), "construction"},
    Group {QLatin1StringView("build system"),
           QT_TRANSLATE_NOOP("update::ReleaseNotes", "Build system"), "build"},
    Group {QLatin1StringView("style"), QT_TRANSLATE_NOOP("update::ReleaseNotes", "Style"), "brush"},
    Group {QLatin1StringView("revert"), QT_TRANSLATE_NOOP("update::ReleaseNotes", "Reverted"),
           "undo"},
    Group {QLatin1StringView("tests"), QT_TRANSLATE_NOOP("update::ReleaseNotes", "Tests"),
           "science"},
    Group {QLatin1StringView("miscellaneous chores"),
           QT_TRANSLATE_NOOP("update::ReleaseNotes", "Maintenance"), "handyman"},
    Group {QLatin1StringView("continuous integrations"),
           QT_TRANSLATE_NOOP("update::ReleaseNotes", "Continuous integration"), "account_tree"},
};

struct Draft
{
    QString title;
    QStringList notes;
};

const QRegularExpression &headingPattern()
{
    static const QRegularExpression pattern(QStringLiteral("^#{2,6}\\s+(.*?)\\s*#*$"));
    return pattern;
}

const QRegularExpression &bulletPattern()
{
    static const QRegularExpression pattern(QStringLiteral("^[-*+]\\s+(.*)$"));
    return pattern;
}

const QRegularExpression &scopePattern()
{
    static const QRegularExpression pattern(QStringLiteral("^\\*\\*\\(([^)]+)\\)\\*\\*\\s*"));
    return pattern;
}

const QRegularExpression &breakingPattern()
{
    static const QRegularExpression pattern(QStringLiteral("^\\[\\*\\*breaking\\*\\*\\]\\s*"),
                                            QRegularExpression::CaseInsensitiveOption);
    return pattern;
}

const QRegularExpression &commitPattern()
{
    static const QRegularExpression pattern(
        QStringLiteral("\\s*-?\\s*\\(\\[([0-9a-fA-F]{7,40})\\]\\((https://[^)\\s]+)\\)\\)\\s*$"));
    return pattern;
}

update::ReleaseNote noteFrom(QString text)
{
    update::ReleaseNote note;

    if (const QRegularExpressionMatch match = scopePattern().match(text); match.hasMatch()) {
        note.scope = match.captured(1).trimmed();
        text.remove(0, match.capturedLength());
    }
    if (const QRegularExpressionMatch match = breakingPattern().match(text); match.hasMatch()) {
        note.breaking = true;
        text.remove(0, match.capturedLength());
    }
    if (const QRegularExpressionMatch match = commitPattern().match(text); match.hasMatch()) {
        note.commit = match.captured(1).left(7).toLower();
        note.commitUrl = QUrl(match.captured(2));
        text.truncate(match.capturedStart());
    }

    note.text = text.trimmed();
    return note;
}

const Group *groupNamed(const QString &title)
{
    const QString key = title.toLower();
    for (const Group &group : kGroups) {
        if (key == group.key)
            return &group;
    }
    return nullptr;
}

update::ReleaseSection sectionFrom(const Draft &draft)
{
    update::ReleaseSection section;
    section.icon = QString::fromLatin1(kFallbackIcon);
    section.title = draft.title;

    if (const Group *group = groupNamed(draft.title)) {
        section.title = QCoreApplication::translate(kContext, group->title);
        section.icon = QString::fromLatin1(group->icon);
    }

    for (const QString &text : draft.notes) {
        update::ReleaseNote note = noteFrom(text);
        if (note.text.isEmpty())
            continue;
        section.scoped = section.scoped || !note.scope.isEmpty();
        section.notes.append(std::move(note));
    }
    return section;
}

}

namespace update::releaseNotes {

QList<ReleaseSection> parse(const QString &markdown)
{
    QList<Draft> drafts;
    bool continuing = false;

    const QStringList lines = markdown.split(QLatin1Char('\n'));
    for (const QString &raw : lines) {
        const QString line = raw.trimmed();
        if (line.isEmpty()) {
            continuing = false;
            continue;
        }
        if (line.startsWith(QLatin1String("# "))) {
            continuing = false;
            continue;
        }
        if (const QRegularExpressionMatch match = headingPattern().match(line); match.hasMatch()) {
            drafts.append(Draft {match.captured(1), {}});
            continuing = false;
            continue;
        }
        if (drafts.isEmpty())
            drafts.append(Draft {});

        QStringList &notes = drafts.last().notes;
        if (const QRegularExpressionMatch match = bulletPattern().match(line); match.hasMatch())
            notes.append(match.captured(1));
        else if (continuing && !notes.isEmpty())
            notes.last() += QLatin1Char(' ') + line;
        else
            notes.append(line);
        continuing = true;
    }

    QList<ReleaseSection> sections;
    for (const Draft &draft : std::as_const(drafts)) {
        ReleaseSection section = sectionFrom(draft);
        if (!section.notes.isEmpty())
            sections.append(std::move(section));
    }
    return sections;
}

}
