/************************************************************************
 **
 **  @file   otemplateeditormenu.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   18/09/2026
 **
 **  @brief  The left menu of a template editor, and the pages it opens.
 **
 **  @copyright
 **  Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>
 **
 **  This program is free software: you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation, either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  This program is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **
 *************************************************************************/

#ifndef OTEMPLATEEDITORMENU_H
#define OTEMPLATEEDITORMENU_H

#include <QString>
#include <QWidget>

class QListWidget;
class QStackedWidget;

/**
 * @brief A column of names on the left, the pages they open on the right.
 *
 * **Both editors are built this way**, and neither adds anything to it: a cover page's editor and
 * a cartouche's editor differ in what their pages hold, not in how one gets to them. So the pair
 * — the list and the stack — lives in the shared socle.
 *
 * **Rows and pages are counted separately, and this does not assume they match.** A page is added
 * with its row, and both are numbered in the order they arrive; which row opens which page is the
 * family's own rule. The cover page's rows and pages happen to be in the same order; the
 * cartouche's are not — its library is the first row and its editor page the second, where the
 * pages are stacked the other way round.
 *
 * **What it deliberately does not decide is which row is highlighted.** The cover page highlights
 * the row of the page on screen; the cartouche highlights one row for « Create new » only while a
 * template nobody has saved is being written, and none at all once a file exists. That rule is
 * about a document, not about a menu, so the family derives the row and hands it over.
 *
 * The names of the rows are the family's too, for the same reason: « Existing templates » is a
 * sentence belonging to the editor that says it, and its translation lives with the editor's other
 * sentences. **A family must name its rows as it builds them**, not only on a language change: an
 * editor that is retranslated when the language changes and not when it is shown would otherwise
 * come up as a column of nothing.
 */
class OTemplateEditorMenu : public QWidget
{
    Q_OBJECT

public:
    explicit OTemplateEditorMenu(QWidget *parent = nullptr);

    /// Adds a page and the row that opens it. Rows are in the order the pages are added.
    void AddPage(QWidget *page);

    /// Names a row. A row that does not exist is ignored, so a family may name them in any order.
    void SetRowLabel(int row, const QString &label);

    /// Shows a page by its index. An index with no page is ignored.
    void ShowPage(int page);

    /// The page on screen, or -1 when there is none.
    auto CurrentPage() const -> int;

    /// Highlights one row, or clears the highlight with -1. The caller derives the row.
    void HighlightRow(int row);

signals:
    /// A row was clicked — what opening that page takes is the family's business.
    void RowClicked(int row);

private:
    QListWidget *m_rows{nullptr};
    QStackedWidget *m_pages{nullptr};
};

#endif // OTEMPLATEEDITORMENU_H
