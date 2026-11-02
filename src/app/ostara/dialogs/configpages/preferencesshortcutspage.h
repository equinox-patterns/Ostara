/************************************************************************
 **
 **  @file   preferencesshortcutspage.h
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   9 8, 2026
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2023 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
 **  Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>
 **  Modified for Ostara (fork of Valentina) on 26/09/2026.
 **
 **  Valentina is free software: you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation, either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  Valentina is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **
 *************************************************************************/

#ifndef PREFERENCESSHORTCUTSPAGE_H
#define PREFERENCESSHORTCUTSPAGE_H

#include "../vmisc/vabstractshortcutmanager.h"

#include <QList>
#include <QSet>
#include <QStringList>
#include <QVector>
#include <QWidget>

namespace Ui
{
class PreferencesShortcutsPage;
}

/**
 * @brief The keyboard shortcuts page.
 *
 * It was one tab of the old « Configuration » page; it is a page of its own now, so the list can be
 * reached in one click and does not have to share the room with the general settings.
 *
 * The rows are grouped in sections, folded by a caret. The tools are **one** section, not the groups of the
 * toolbar: those belong to the user — she reorders them, renames them, moves a tool from one to another — and a
 * second grouping here would start drifting the day she does. A shortcut is set once; the list only has to be
 * predictable.
 */
class PreferencesShortcutsPage : public QWidget
{
    Q_OBJECT // NOLINT

public:
    explicit PreferencesShortcutsPage(QWidget *parent = nullptr);
    ~PreferencesShortcutsPage() override;

    auto Apply() -> QStringList;

protected:
    void changeEvent(QEvent *event) override;

    /** @brief Split the two columns down the middle, and hear the reader's own widths.
     *
     *  Watched: the **viewport** of the table, not this page. A page is resized before its layout has given the
     *  table its final width, so splitting there divided a width the table did not have yet — 150 px of columns
     *  in a 1200 px table, and nothing ever corrected it. The viewport, itself, only ever reports its real size.
     *
     *  Watched too: the header, whose press is the only sure sign that a divider is being dragged. From that
     *  moment the widths are the reader's and are left alone. */
    auto eventFilter(QObject *watched, QEvent *event) -> bool override;

protected slots:
    void ShortcutCellClicked(int row, int column);

private slots:
    void ShortcutCellDoubleClicked(int row, int column);

private:
    // cppcheck-suppress unknownMacro
    Q_DISABLE_COPY_MOVE(PreferencesShortcutsPage) // NOLINT

    /** @brief What a row of the table stands for. A section row is the heading above its actions: it wears the
     *  section's name, folds it away with a caret, and carries nothing to edit. */
    enum class RowKind : quint8
    {
        Action,
        Section
    };

    /** @brief The sections, in the order they are read. */
    enum class Section : quint8
    {
        File,
        Edit,
        View,
        Pieces,
        Modes,
        Tools,
        Export,
        LAST_ONE_DO_NOT_USE
    };

    struct Row
    {
        RowKind kind{RowKind::Action};
        Section section{Section::Tools};
        int shortcutIndex{-1};
    };

    Ui::PreferencesShortcutsPage *ui;
    QList<QStringList> m_transientShortcuts{};

    /** @brief The shortcuts of the manager, in its own order. A row's index in this list is its shortcut. */
    QVector<VShortcutAction> m_actions{};

    /** @brief The rows on screen, in display order. Built by FillTable(), read back by the slots. */
    QVector<Row> m_rows{};

    /** @brief The sections the reader has folded away. Remembered from one session to the next. */
    QSet<Section> m_collapsed{};

    /** @brief The section row under the cursor, or -1. The caret is swapped by hand: Qt asks the icon for the
     *  Selected state only, never for Active, so a hovered row would keep the theme's glyph on the accent
     *  background the stylesheet gives it. */
    int m_hoveredSectionRow{-1};

    bool m_userChoseColumnWidths{false};

    /** @brief True while the halves are being written, so that the resize they may cause cannot come back here. */
    bool m_splittingColumns{false};

    void InitShortcuts(bool defaults = false);
    void FillTable();
    void UpdateShortcutsTable();
    void ApplyFold();
    void ToggleSection(int row);
    void HoverSectionRow(int row);
    void RepaintSectionCaret(int row);
    void LoadFoldedSections();
    void SaveFoldedSections() const;

    /** @brief True when @p action answers @p needle: its name, its technical key, or its shortcut. */
    auto MatchesFilter(VShortcutAction action, int shortcutIndex, const QString &needle) const -> bool;

    /** @brief The section an action belongs to. Every tool falls to Tools; the rest is named one by one. */
    static auto SectionOf(VShortcutAction action) -> Section;
    static auto SectionName(Section section) -> QString;

    /** @brief The section's stable name, untranslated: what the remembered state stores. */
    static auto SectionKey(Section section) -> QString;

    void SplitColumnsEvenly();
};

#endif // PREFERENCESSHORTCUTSPAGE_H
