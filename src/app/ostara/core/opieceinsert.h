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
#include "opiecemark.h"

#include "../vtools/undocommands/vundocommand.h"

#include <QDomElement>
#include <QHash>
#include <QPointF>
#include <QString>
#include <QVector>

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
 * @brief What a piece asks before it enters: the name it would wear, the aliases it carries, and the table proposed.
 *
 * Read from the pattern alone, and from nothing of the piece's own: the name a block may wear is the pattern's to
 * say, and so is the mark, since the marks of two pieces entering one after the other have to differ.
 */
struct Proposal
{
    QString instanceName{};               ///< le nom que le bloc prendrait dans le patron.
    int number{0};                        ///< le numéro de marque que le patron donnerait.
    bool collides{false};                 ///< vrai quand le patron répond déjà à un des noms de la pièce.
    QStringList roles{};                  ///< les noms de rôle de la pièce : hors du bloc, donc à dire à part.
    QVector<OPieceMark::Alias> aliases{}; ///< les alias de la pièce, dans l'ordre du fichier.
    QHash<QString, QString> table{};      ///< les noms d'avant et d'après — vide quand rien ne collide.

    /// Le vrai quand la proposition a pu être lue : le patron et son conteneur étaient là.
    auto IsReady() const -> bool { return number > 0; }
};

/**
 * @brief The keys @p data answers to, one per object it holds — what an alias of a piece would overwrite in silence.
 *
 * Read here rather than twice: the proposal, the mark and the window that validates a typed alias all read the
 * same keys, so a name the window accepts is a name the marking does not have to fear.
 */
auto Keys(VContainer *data) -> QSet<QString>;

/**
 * @brief What @p piece would ask of @p doc, without touching anything — what a dialog is shown, and what a caller
 * that asks nobody passes straight to Prepare.
 *
 * @param doc the pattern, asked for two things: the highest mark it already carries, and whether it answers to the
 *            piece's names.
 * @param data the pattern's container, which holds one key per object — the keys an alias would overwrite.
 */
auto Propose(const OPieceDocument &piece, VPattern *doc, VContainer *data, const QString &instanceName) -> Proposal;

/**
 * @brief Make @p piece ready to enter the pattern @p proposal was read from, without touching anything.
 *
 * Three things, in this order, and the order matters: the **origin** is written while the block still carries the
 * piece's name; the block is **renamed** to its instance name; the names @p proposal decides are **marked**; and
 * its ids are **renewed** by the schema's own rule.
 *
 * @param data the pattern's container, asked for the counter the new ids come from.
 * @param anchorTarget where the construction must start, **in the pattern's own unit** — the place a base point of
 *                     the block takes, and every other object with it. The caller decides it, because it is the
 *                     canvas that knows what is already drawn (see MainWindow::PositionBeyondDrawn).
 * @return a block ready to insert, or a block-less result when there is nothing to prepare.
 */
auto Prepare(const OPieceDocument &piece, VContainer *data, const Proposal &proposal,
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
