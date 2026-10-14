/************************************************************************
 **
 **  @file   ocoverpagetemplatepreview.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   18/09/2026
 **
 **  @brief  Preview of a cover page — one white page, and the skeleton it is filled on.
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

#ifndef OCOVERPAGETEMPLATEPREVIEW_H
#define OCOVERPAGETEMPLATEPREVIEW_H

#include "core/ocoverpagetemplate.h"
#include "core/oseamallowancelibrary.h"
#include "core/osymbollibrary.h"

#include <QPointer>
#include <QWidget>

class OSizeTable;
class VAbstractPattern;

/**
 * @brief Shows the page a cover page is written on, and what the template puts on it.
 *
 * **One page, not a plan.** The assembly marks are previewed as six sheets side by side, because
 * what they decorate is a plan; a cover page is a single leaf, and the preview shows one.
 *
 * **The page is 19 × 25.9 cm and keeps its proportions** whatever the pane is given: it is the
 * shape of a page that is being shown, and a page stretched to fill its pane would not be one.
 * The page is the preview's own business and not a property of the template - one template is
 * written for the PDF export of any pattern, and nothing chosen here is saved.
 *
 * **What it draws today is the skeleton, not the drawing.** Each information stands for itself as
 * a line, as the Information block preview shows its sections, so that the arrangement can be
 * judged before anything is painted: which band an information sits in, how the middle splits,
 * and how the whole stands on the page. What each information will actually draw comes after.
 */
class OCoverPageTemplatePreview : public QWidget
{
    Q_OBJECT

public:
    explicit OCoverPageTemplatePreview(QWidget *parent = nullptr);

    /** @brief The template to show. Nothing is drawn but the page while it is null. */
    void SetTemplate(const OCoverPageTemplate *tpl);

    /** @brief Redraws. Called when the template changed under it. */
    void Refresh();

    /**
     * @brief The sizes of the pattern, for the informations that state one. Not owned.
     *
     * A cover page is written for a pattern, and an information that names a size cannot name one
     * without the table that declares it. Null is not an error: it is a template being written
     * before any pattern is chosen, which is exactly what the editor is.
     */
    void SetTable(const OSizeTable *table);

    /**
     * @brief The symbol library, for the legend. Not owned.
     *
     * **Handed over for the same reason the table is**: which symbols a legend holds is a question
     * about a library and about which of them are permanent — neither of which is a property of a
     * template. Null is not an error: it is a template written on a machine whose library has not
     * been read yet, and the legend then draws its title alone.
     */
    void SetSymbols(const OSymbolLibrary::Library *symbols);

    /**
     * @brief The seam-allowance library, for the margins a pattern is cut with. Not owned.
     *
     * **Handed over for the same reason the legend's is**: what a margin is called is a question
     * about a library, and which margins the page has to name is a question about the pattern the
     * editor is opened beside — neither of which is a property of a template. Null is not an error:
     * it is a template written before its library has been read, and the information then draws its
     * heading alone.
     */
    void SetSeamAllowances(const OSeamAllowanceLibrary::Library *seamAllowances);

    /** @brief What the pane asks for when it is left to size itself. */
    auto minimumSizeHint() const -> QSize override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    /**
     * @brief Subscribes to the pattern the application has open, once, the first time there is one.
     *
     * What a page reads from a pattern is written elsewhere — by the Cover page dock, and by the
     * pattern's own toolbar for the name — so the page has to hear about it or it would show a
     * pattern that no longer exists.
     */
    void FollowPattern();

    const OCoverPageTemplate *m_template{nullptr};
    const OSizeTable *m_table{nullptr};
    const OSymbolLibrary::Library *m_symbols{nullptr};
    const OSeamAllowanceLibrary::Library *m_seamAllowances{nullptr};

    /// The pattern being followed, so the subscription is made once and not once per redraw.
    QPointer<VAbstractPattern> m_following{};
};

#endif // OCOVERPAGETEMPLATEPREVIEW_H
