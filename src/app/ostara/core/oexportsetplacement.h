/************************************************************************
 **
 **  @file   oexportsetplacement.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   16/09/2026
 **
 **  @brief  Undo command writing one placement of a nesting generation. Keeps
 **          both ends of a hand gesture and rewrites the store on undo and redo.
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

#ifndef OEXPORTSETPLACEMENT_H
#define OEXPORTSETPLACEMENT_H

#include <QUndoCommand>

#include "../oexportnesting.h"

class OExportOutputStore;

/// @brief One hand-made move or turn of a footprint, undoable.
///
/// The placement lives in the model, not on an item. Build() recreates every item it stamped
/// on each repaint, so a command holding a QGraphicsItem would be pointing at a deleted object
/// by the time it was asked to undo anything. What is kept instead is where the footprint was
/// and where it went — two plain values — plus the generation and the index that name which
/// footprint of the plan they belong to.
///
/// It writes to the store rather than to the scene, and that is the only reason an undo
/// survives a rebuild: restoring a transform would be erased by the next Build, which reads
/// the model and nothing else.
///
/// A command never repaints and never reaches for a window. The store is not a QObject and
/// cannot announce anything; the window listens to the stack's indexChanged, as it does for
/// every other stack in the application.
class OExportSetPlacement : public QUndoCommand
{
public:
    /// @param store    the session's generations. Must outlive the command, which it does:
    ///                 both are owned by MainWindow and the store is a plain member, not a
    ///                 pointer that could be replaced under us.
    /// @param outputId the generation, @p index the placement within its result. Together they
    ///                 are the identity of one footprint, and they survive a rebuild, which a
    ///                 pointer would not.
    /// @param from     the placement the gesture started at, @p to the one it ended at.
    /// @param turned   whether the gesture was a rotation. It decides the text only — the menu
    ///                 shows it as "Undo <text>", and telling a user they moved a piece they
    ///                 turned is a small lie that costs one word to avoid.
    OExportSetPlacement(OExportOutputStore *store, quint32 outputId, int index,
                        const OExportPlacement &from, const OExportPlacement &to, bool turned,
                        QUndoCommand *parent = nullptr);

    void undo() override;
    void redo() override;

private:
    /// The one write, shared by undo() and redo(), which differ only in the end they apply.
    /// Neither needs a guard of its own: OExportOutputStore::SetPlacement is a documented
    /// no-op on an unknown generation or an out-of-range index, so a command that outlived its
    /// plan — the pattern was closed, the generation was refreshed away — falls silent rather
    /// than resurrecting anything.
    void Apply(const OExportPlacement &placement);

    OExportOutputStore *m_store{nullptr};
    quint32 m_outputId{0};
    int m_index{-1};
    OExportPlacement m_from{};
    OExportPlacement m_to{};
};

#endif // OEXPORTSETPLACEMENT_H
