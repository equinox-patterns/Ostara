/************************************************************************
 **
 **  @file   opieceinsert.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   12/10/2026
 **
 **  @brief  A library piece entering the open pattern: made ready, and put in as one gesture.
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
#ifndef OPIECEINSERT_H
#define OPIECEINSERT_H

#include "opiecedocument.h"

#include "../vtools/undocommands/vundocommand.h"

#include <QDomElement>
#include <QHash>
#include <QPointF>
#include <QString>

class VContainer;
class VPattern;

namespace OPieceInsert
{
/**
 * @brief A library piece made ready to enter a pattern.
 *
 * Everything that must happen **before** the block meets the living document, because doing it afterwards would
 * mean renaming inside the pattern — and a rename inside the pattern rewrites the formulas of every block, for a
 * change that concerns one piece.
 */
struct Prepared
{
    QDomElement block{};               ///< le `<draw>` prêt : renommé, marqué, ses ids renouvelés.
    QHash<QString, QString> renamed{}; ///< les noms d'avant et d'après — vide quand rien ne collide.
    QString instanceName{};            ///< le nom que le bloc portera dans le patron.

    auto IsReady() const -> bool { return not block.isNull(); }
};

/**
 * @brief Make @p piece ready to enter @p doc under the name @p instanceName, without touching anything.
 *
 * Four things, in this order, and the order matters: the **origin** is written while the block still carries the
 * piece's name; the block is **renamed** to its instance name; its names are **marked** — all of them or none,
 * decided on what the pattern already answers to; and its ids are **renewed** by the schema's own rule.
 *
 * @param doc the pattern the piece would enter, asked for two things: the highest mark it already carries, and
 *            whether it answers to the piece's names.
 * @param data the pattern's container, asked for the keys it answers to and for the counter the new ids come from.
 * @param anchorTarget where the construction must start, **in the pattern's own unit** — the place a base point of
 *                     the block takes, and every other object with it. The caller decides it, because it is the
 *                     canvas that knows what is already drawn (see MainWindow::PositionBeyondDrawn).
 * @return a block ready to insert, or a block-less result when there is nothing to prepare.
 */
auto Prepare(const OPieceDocument &piece, VPattern *doc, VContainer *data, const QString &instanceName,
             const QPointF &anchorTarget) -> Prepared;

/**
 * @brief Inserting a prepared piece, and everything it brings, as **one** step a single Ctrl+Z takes back.
 *
 * A piece is not one node. It brings increments into tables at the root of the pattern, groups to hold them, the
 * names of its lines into the root as well, and names of role into the container — none of which passes through an
 * undo command anywhere in the product: the increments are written straight into the DOM, by the dialog as much as
 * by this import. So the command does not delegate its undo to anyone: it **saves the five sections of the root**
 * the import writes into, and puts them back as they were. What it removes, it removes entirely; what it restores,
 * it restores exactly.
 */
class AddPieceFromLibrary : public VUndoCommand
{
    Q_OBJECT // NOLINT

public:
    AddPieceFromLibrary(const Prepared &prepared, const OPieceDocument &piece, VPattern *doc, VContainer *data,
                        QUndoCommand *parent = nullptr);
    ~AddPieceFromLibrary() override = default;

    void undo() override;
    void redo() override;

    /// @brief The name the inserted block wears — meaningful once the command has run once.
    auto InstanceName() const -> QString;

private:
    Q_DISABLE_COPY_MOVE(AddPieceFromLibrary) // NOLINT

    /// @brief Keep the five sections of the root the import is about to write into.
    void SaveSections();

    /// @brief Put those five sections back exactly as they were, whatever the import left.
    void RestoreSections();

    QString m_instanceName{};
    QDomElement m_block{};
    QHash<QString, QString> m_renamed{};
    OPieceDocument m_piece{};     ///< a copy the command owns: a redo replays the import on the same piece.
    VPattern *m_pattern{nullptr}; ///< the document, in its own type.
    VContainer *m_data{nullptr};
    int m_indexPatternBlock{-1};

    QDomElement m_increments{};
    QDomElement m_incrementGroups{};
    QDomElement m_previewCalculations{};
    QDomElement m_previewCalculationGroups{};
    QDomElement m_lineNames{};
};
} // namespace OPieceInsert

#endif // OPIECEINSERT_H
