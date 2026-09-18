/************************************************************************
 **
 **  @file   otemplateguard.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   18/09/2026
 **
 **  @brief  The question asked before unsaved work is thrown away.
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

#ifndef OTEMPLATEGUARD_H
#define OTEMPLATEGUARD_H

#include <QString>

class QWidget;

/**
 * @brief What the guard needs of the document an editor is showing: whether there is unsaved work,
 *        and how to save it.
 *
 * **Two methods, because those are the two things the question needs** — not a base class for the
 * editors, which would have made each of them the other's shape to share one dialog. Any editor
 * that can answer these two can ask the question, and a third family costs two methods rather
 * than a second copy of the prompt.
 */
class OTemplateDocumentEdits
{
public:
    OTemplateDocumentEdits() = default;
    virtual ~OTemplateDocumentEdits() = default;

    OTemplateDocumentEdits(const OTemplateDocumentEdits &) = default;
    auto operator=(const OTemplateDocumentEdits &) -> OTemplateDocumentEdits & = default;

    /** @brief True while the document holds something that is not on disk. */
    virtual auto HasUnsavedChanges() const -> bool = 0;

    /**
     * @brief Writes the document, asking whatever it needs to ask.
     *
     * It reports nothing: what it did is read back through HasUnsavedChanges, which is the same
     * question that was asked before — a save that did not happen, because the name was taken or
     * the folder could not be written, has left the change unsaved and says so there.
     */
    virtual void SaveDocument() = 0;
};

namespace OTemplateGuard
{
/**
 * @brief Asks before throwing away unsaved work. True when it is safe to carry on.
 *
 * Three answers, and the one the keyboard reaches first is the one that keeps the work. Choosing
 * to save and failing to does not carry on: leaving then would lose exactly what the user asked
 * to keep.
 *
 * @param title what the document is called in the prompt, in the caller's own words.
 */
auto ConfirmDiscardChanges(QWidget *parent, const QString &title, OTemplateDocumentEdits *document) -> bool;
} // namespace OTemplateGuard

#endif // OTEMPLATEGUARD_H
