/************************************************************************
 **
 **  @file   otemplateguard.cpp
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

#include "otemplateguard.h"

#include <QCoreApplication>
#include <QMessageBox>

//---------------------------------------------------------------------------------------------------------------------

auto OTemplateGuard::ConfirmDiscardChanges(QWidget *parent, const QString &title,
                                           OTemplateDocumentEdits *document) -> bool
{
    // Nothing to lose is nothing to ask about.
    if (document == nullptr || !document->HasUnsavedChanges())
    {
        return true;
    }

    const auto answer = QMessageBox::warning(
        parent, title,
        QCoreApplication::translate("OTemplateDocumentEdits",
                                    "This template has been changed and not saved.\n\n"
                                    "Save it before leaving it?"),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);

    if (answer == QMessageBox::Cancel)
    {
        return false;
    }

    if (answer == QMessageBox::Discard)
    {
        // The answer is carried out, not merely recorded. A document that cannot be put back —
        // its file gone, unreadable — keeps its work and its flag: the caller carries on, and the
        // next close asks again. Saying nothing there would be the silent loss the question exists
        // to prevent.
        document->DiscardUnsavedChanges();
        return true;
    }

    document->SaveDocument();

    // A save that did not happen — a name already taken, a folder that cannot be written — has
    // left the change unsaved, and carrying on now would lose the very thing the user chose to
    // keep.
    return !document->HasUnsavedChanges();
}
