/************************************************************************
 **
 **  @file   opiecegroupdialog.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   01/09/2026
 **
 **  @brief  Creation dialog for a piece group: name, number, cut, free text
 **          and graded alignment, reusing OPiecePieceStep.
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

#ifndef OPIECEGROUPDIALOG_H
#define OPIECEGROUPDIALOG_H

#include <QDialog>

#include "../vpatterndb/opiecegroup.h"

class QDialogButtonBox;
class OPiecePieceStep;
class VContainer;

/**
 * @brief The OPieceGroupDialog class — creation dialog for a piece group.
 *
 * No default values: the user fills every field (name is required, the OK
 * button stays disabled until it is non-empty). Reuses OPiecePieceStep
 * (name, number + total, cut x3, free text, graded alignment).
 */
class OPieceGroupDialog final : public QDialog
{
    Q_OBJECT // NOLINT

public:
    explicit OPieceGroupDialog(const VContainer *data, int memberCount, QWidget *parent = nullptr);
    ~OPieceGroupDialog() override = default;

    /** @brief Builds the group from the step fields (id / pieceIds filled by the caller). */
    auto GetGroup() const -> OPieceGroup;

    /** @brief Layout flags chosen for the group. A group carries none of them: the caller writes them into every
     *         member piece (see ODetailsGroupWidget, which edits them the same way). */
    auto IsForceFlipping() const -> bool;
    auto IsSymmetricalCopy() const -> bool;

private:
    void UpdateButtons(const QString &name);

    // cppcheck-suppress unknownMacro
    Q_DISABLE_COPY_MOVE(OPieceGroupDialog) // NOLINT

    OPiecePieceStep *m_step{nullptr};
    QDialogButtonBox *m_buttonBox{nullptr};
};

#endif // OPIECEGROUPDIALOG_H
